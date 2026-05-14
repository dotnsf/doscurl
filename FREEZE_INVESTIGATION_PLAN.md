# DOSCurl 2回目実行フリーズ問題 - 調査・解決プラン

## 問題の概要

**症状**: doscurl.exeを1回目実行すると正常に動作するが、2回目以降の実行で"Initializing TCP/IP stack..."の段階でフリーズする

**考えられる原因**:
1. mTCPスタックが完全にクリーンアップされていない
2. パケットドライバーの状態が残っている
3. 割り込みハンドラーが解放されていない

---

## Phase 1: 問題の特定 - フリーズ箇所の正確な特定

### 目的
Utils::parseEnv()とUtils::initStack()のどちらでフリーズしているかを特定

### 実施内容
1. **デバッグ出力の追加**
   ```cpp
   fprintf(stderr, "Calling Utils::parseEnv()...\n");
   int rc = Utils::parseEnv();
   fprintf(stderr, "Utils::parseEnv() returned: %d\n", rc);
   
   fprintf(stderr, "Calling Utils::initStack()...\n");
   rc = Utils::initStack(2, TCP_SOCKET_RING_SIZE, ctrlBreakHandler, ctrlBreakHandler);
   fprintf(stderr, "Utils::initStack() returned: %d\n", rc);
   ```

2. **テスト実行**
   - 1回目実行: どこまで進むか確認
   - 2回目実行: どこでフリーズするか確認

### 期待される結果
- フリーズする正確な関数を特定できる

---

## Phase 2: mTCPスタックのクリーンアップ検証

### 目的
現在スキップしているUtils::endStack()が原因かどうかを確認

### 実施内容

#### 2-1: Utils::endStack()を呼ぶバージョンのテスト
```cpp
static void shutdown(int rc) {
  if (sock != NULL) {
    sock->close();
    TcpSocketMgr::freeSocket(sock);
    sock = NULL;
  }
  
  if (responseData) free(responseData);
  if (recvBuffer) free(recvBuffer);
  if (httpRequest) free(httpRequest);
  if (base64Buffer) free(base64Buffer);
  
  // 試しにUtils::endStack()を呼んでみる
  fprintf(stderr, "Calling Utils::endStack()...\n");
  Utils::endStack();
  fprintf(stderr, "Utils::endStack() completed\n");
  
  exit(rc);
}
```

#### 2-2: テスト実行
- 1回目: "heap is corrupted!"が出るか確認
- 2回目: フリーズするか、正常に動作するか確認

### 期待される結果
- Utils::endStack()を呼ぶと2回目が動作する → クリーンアップ不足が原因
- Utils::endStack()を呼んでも2回目がフリーズ → 別の原因

---

## Phase 3: Utils::endStack()の影響調査

### 目的
Utils::endStack()内部で何が行われているかを理解し、必要な部分だけを実行

### 実施内容

#### 3-1: mTCPソースコードの確認
`C:\mTCP\src\TCPLIB\utils.cpp`のendStack()関数を確認：
- パケットドライバーの解放処理
- 割り込みハンドラーの解放処理
- メモリの解放処理
- その他のクリーンアップ処理

#### 3-2: 選択的クリーンアップの実装
ヒープチェックを除く必要な処理だけを実行：
```cpp
// 例: パケットドライバーの解放だけを行う
extern "C" void _w32_pkt_release(void);
_w32_pkt_release();
```

### 期待される結果
- 必要最小限のクリーンアップ処理を特定できる

---

## Phase 4: 割り込みハンドラーの状態確認

### 目的
ctrlBreakHandlerが正しく解放されているかを確認

### 実施内容

#### 4-1: 割り込みハンドラーの登録状態を確認
```cpp
// 初期化前
fprintf(stderr, "Before initStack: checking interrupt handlers...\n");

// 初期化
Utils::initStack(2, TCP_SOCKET_RING_SIZE, ctrlBreakHandler, ctrlBreakHandler);

// 終了前
fprintf(stderr, "Before shutdown: checking interrupt handlers...\n");
```

#### 4-2: 明示的な割り込みハンドラーの解放
```cpp
// DOS割り込みベクタの復元が必要な場合
// (mTCPのドキュメントを参照)
```

### 期待される結果
- 割り込みハンドラーが2重登録されていないか確認できる

---

## Phase 5: パケットドライバーの状態確認

### 目的
パケットドライバーが正しく解放されているかを確認

### 実施内容

#### 5-1: パケットドライバーの状態確認
```cpp
// Utils::initStack()の前にパケットドライバーの状態を確認
fprintf(stderr, "Checking packet driver status...\n");
if (_watt_do_exit) {
  fprintf(stderr, "Packet driver already initialized!\n");
} else {
  fprintf(stderr, "Packet driver not initialized\n");
}
```

#### 5-2: 明示的なパケットドライバーの解放
mTCPのドキュメントを参照して、パケットドライバーを明示的に解放する方法を調査

### 期待される結果
- パケットドライバーの状態が2回目実行時に異常かどうか確認できる

---

## Phase 6: 解決策の実装とテスト

### 目的
Phase 1-5で特定した原因に基づいて解決策を実装

### 実施内容

#### 6-1: 解決策のパターン

**パターンA: Utils::endStack()を呼ぶが、ヒープチェックを無効化**
```cpp
// doscurl.cfgで完全にヒープチェックを無効化
#define UTILS_CHECK_HEAP (0)
#define UTILS_HEAP_CHECK_LEVEL (0)
```

**パターンB: 選択的クリーンアップ**
```cpp
static void shutdown(int rc) {
  // ソケットのクリーンアップ
  if (sock != NULL) {
    sock->close();
    TcpSocketMgr::freeSocket(sock);
    sock = NULL;
  }
  
  // メモリの解放
  if (responseData) free(responseData);
  if (recvBuffer) free(recvBuffer);
  if (httpRequest) free(httpRequest);
  if (base64Buffer) free(base64Buffer);
  
  // パケットドライバーの解放（必要な場合）
  // _w32_pkt_release();
  
  // 割り込みハンドラーの解放（必要な場合）
  // restore_interrupt_handlers();
  
  exit(rc);
}
```

**パターンC: プログラム構造の変更**
```cpp
// メインループを関数化して、複数回呼び出せるようにする
int performRequest(const char *url) {
  // 既存のリクエスト処理
}

int main(int argc, char *argv[]) {
  // 初期化は1回だけ
  Utils::parseEnv();
  Utils::initStack(...);
  
  // リクエスト実行
  int result = performRequest(url);
  
  // クリーンアップ
  Utils::endStack();
  return result;
}
```

#### 6-2: テスト計画
1. 各パターンをビルド
2. 1回目実行のテスト
3. 2回目実行のテスト
4. 3回目以降のテスト
5. 異なるURLでのテスト

### 期待される結果
- 2回目以降も正常に実行できるバージョンが完成する

---

## 優先順位と推奨アプローチ

### 最優先: Phase 1 → Phase 2
まずフリーズ箇所を特定し、Utils::endStack()の影響を確認することが最も重要です。

### 推奨される調査順序
1. **Phase 1**: デバッグ出力でフリーズ箇所を特定（30分）
2. **Phase 2**: Utils::endStack()を呼ぶテスト（15分）
3. **Phase 3 or 4 or 5**: Phase 2の結果に基づいて選択（1-2時間）
4. **Phase 6**: 解決策の実装（1-2時間）

### 成功の判断基準
- doscurl.exeを連続10回実行しても正常に動作する
- 異なるURLに対しても連続実行が可能
- メモリリークがない

---

## 参考情報

### mTCPドキュメント
- `C:\mTCP\src\DOCS\`ディレクトリ内のドキュメント
- 特に`MTCP.TXT`や`CODING.TXT`を参照

### 類似プロジェクトの調査
mTCPを使用している他のプログラム（FTPSRV、TELNET等）がどのようにクリーンアップしているかを確認

---

## 進捗記録

### 実施日時: [記入してください]

#### Phase 1の結果
- フリーズ箇所: 
- 観察された動作:

#### Phase 2の結果
- Utils::endStack()を呼んだ場合の動作:
- 2回目実行の結果:

#### Phase 3-5の結果
- 特定された原因:
- 実施した対策:

#### Phase 6の結果
- 採用した解決策:
- テスト結果:

---

このプランに従って段階的に調査を進めることで、2回目実行フリーズ問題の原因を特定し、解決できるはずです。