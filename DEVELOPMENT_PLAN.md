# DOSCurl 開発計画書

## プロジェクト概要

16bit PC-DOS環境で動作するHTTPクライアント（curlライク）の開発

### 開発環境
- **ホストOS**: Windows 11
- **コンパイラ**: Open Watcom V2 (C:\WATCOM)
- **TCP/IPライブラリ**: WATTCP (wattcpws.lib - small model)
- **テスト環境**: DOSBox-X + mTCP + NE2000エミュレーション
- **対応プロトコル**: HTTP/1.0, HTTP/1.1 (HTTPSは非対応)

### 目標機能
- HTTP GETリクエスト
- HTTP POSTリクエスト（本文指定）
- URLパース機能
- レスポンス表示
- 基本的なエラーハンドリング

---

## Phase 1: プロジェクト構造とビルドシステムの構築

### 目的
開発に必要なディレクトリ構造とビルドシステムを整備

### タスク

#### 1.1 ディレクトリ構造の作成
```
doscurl/
├── src/              # ソースコード
│   ├── main.c        # メインエントリーポイント
│   ├── http.c        # HTTP処理
│   ├── socket.c      # ソケット処理
│   ├── url.c         # URLパーサー
│   └── utils.c       # ユーティリティ関数
├── include/          # ヘッダーファイル
│   ├── doscurl.h     # 共通ヘッダー
│   ├── http.h        # HTTP関連
│   ├── socket.h      # ソケット関連
│   ├── url.h         # URLパーサー
│   └── utils.h       # ユーティリティ
├── build/            # ビルド出力
├── test/             # テストスクリプト
│   └── test.bat      # DOSBox-X用テストバッチ
├── docs/             # ドキュメント
│   └── USAGE.md      # 使用方法
├── Makefile          # Open Watcom用Makefile
└── README.md         # プロジェクト説明
```

#### 1.2 Makefileの作成
- Open Watcom C コンパイラ (wcl) 用の設定
- Small memory model (-ms)
- WATTCP ライブラリのリンク
- デバッグビルドとリリースビルドの分離

#### 1.3 ビルドスクリプトの作成
- `build.bat`: Windows上でのクロスコンパイル用
- `clean.bat`: ビルド成果物のクリーンアップ

### 成果物
- 完全なディレクトリ構造
- 動作するMakefile
- ビルドスクリプト
- 最小限の"Hello World"プログラムがビルド可能

---

## Phase 2: 基本的なネットワーク接続機能の実装

### 目的
WATTCPを使用した基本的なTCP接続を確立

### タスク

#### 2.1 WATTCP初期化処理
- `sock_init()`: WATTCPライブラリの初期化
- パケットドライバの検出
- ネットワーク設定の読み込み

#### 2.2 TCP接続の実装
- `socket()`: ソケット作成
- `connect()`: サーバーへの接続
- `close()`: 接続のクローズ
- タイムアウト処理

#### 2.3 DNS解決機能
- `resolve()`: ホスト名からIPアドレスへの変換
- DNSクエリのタイムアウト処理

#### 2.4 基本的な送受信
- `send()`: データ送信
- `recv()`: データ受信
- バッファ管理

### テスト方法
```c
// 簡単なTCP接続テスト
// example.com:80 に接続して切断
```

### 成果物
- [`socket.c`](doscurl/src/socket.c) と [`socket.h`](doscurl/include/socket.h)
- TCP接続が確立できることを確認
- DOSBox-X上で動作確認

---

## Phase 3: HTTP GETリクエストの実装

### 目的
HTTP/1.0 GETリクエストの送信とレスポンス受信

### タスク

#### 3.1 URLパーサーの実装
- プロトコル（http://）の抽出
- ホスト名の抽出
- ポート番号の抽出（デフォルト80）
- パスの抽出

例: `http://example.com:8080/path/to/resource`
- ホスト: `example.com`
- ポート: `8080`
- パス: `/path/to/resource`

#### 3.2 HTTP GETリクエストの構築
```http
GET /path HTTP/1.0
Host: example.com
User-Agent: DOSCurl/1.0
Connection: close

```

#### 3.3 HTTPレスポンスのパース
- ステータスライン（HTTP/1.0 200 OK）
- ヘッダーのパース
- ボディの抽出と表示

#### 3.4 チャンク転送エンコーディング対応
- `Transfer-Encoding: chunked` の処理
- チャンクサイズの読み取り
- データの結合

### テスト方法
```batch
doscurl http://example.com/
doscurl http://httpbin.org/get
```

### 成果物
- [`http.c`](doscurl/src/http.c) と [`http.h`](doscurl/include/http.h)
- [`url.c`](doscurl/src/url.c) と [`url.h`](doscurl/include/url.h)
- HTTP GETが動作することを確認

---

## Phase 4: HTTP POSTリクエストの実装

### 目的
HTTP POSTリクエストでデータを送信

### タスク

#### 4.1 POSTリクエストの構築
```http
POST /path HTTP/1.0
Host: example.com
User-Agent: DOSCurl/1.0
Content-Type: application/x-www-form-urlencoded
Content-Length: 13
Connection: close

key1=value1
```

#### 4.2 Content-Lengthの計算
- POSTデータのバイト数を正確に計算
- ヘッダーに含める

#### 4.3 POSTデータの送信
- ヘッダー送信後にボディを送信
- バッファリング処理

### テスト方法
```batch
doscurl -X POST -d "name=test&value=123" http://httpbin.org/post
```

### 成果物
- POST機能を追加した [`http.c`](doscurl/src/http.c)
- POSTリクエストが動作することを確認

---

## Phase 5: コマンドライン引数パーサーの実装

### 目的
curlライクなコマンドライン引数を処理

### タスク

#### 5.1 引数パーサーの実装
サポートする引数:
- `URL`: 必須、リクエスト先URL
- `-X METHOD`: HTTPメソッド指定（GET, POST）
- `-d DATA`: POSTデータ
- `-H HEADER`: カスタムヘッダー追加
- `-v`: 詳細出力（verbose）
- `-o FILE`: 出力をファイルに保存
- `--help`: ヘルプ表示

#### 5.2 使用例
```batch
# 基本的なGET
doscurl http://example.com/

# POSTリクエスト
doscurl -X POST -d "key=value" http://example.com/api

# カスタムヘッダー
doscurl -H "Authorization: Bearer token" http://api.example.com/

# ファイル出力
doscurl -o output.txt http://example.com/data.json

# 詳細モード
doscurl -v http://example.com/
```

#### 5.3 ヘルプメッセージ
```
DOSCurl - HTTP client for DOS
Usage: doscurl [options] <URL>

Options:
  -X METHOD     HTTP method (GET, POST)
  -d DATA       POST data
  -H HEADER     Custom header
  -v            Verbose output
  -o FILE       Output to file
  --help        Show this help
```

### 成果物
- [`main.c`](doscurl/src/main.c) の引数処理部分
- 各種オプションが動作することを確認

---

## Phase 6: エラーハンドリングとレスポンス処理の改善

### 目的
堅牢性と使いやすさの向上

### タスク

#### 6.1 エラーハンドリング
- ネットワークエラー（接続失敗、タイムアウト）
- DNS解決エラー
- HTTPエラー（4xx, 5xx）
- メモリ不足エラー
- 不正なURL形式

#### 6.2 エラーメッセージの改善
```
Error: Failed to connect to example.com:80
Error: DNS resolution failed for unknown.host
Error: HTTP 404 Not Found
Error: Invalid URL format
```

#### 6.3 レスポンス処理の改善
- HTTPステータスコードの表示
- レスポンスヘッダーの表示（-vオプション時）
- プログレス表示（大きなファイル用）
- リダイレクト対応（301, 302）

#### 6.4 メモリ管理
- 動的メモリ割り当ての最適化
- メモリリークの防止
- バッファオーバーフローの防止

### 成果物
- エラーハンドリングを強化した全モジュール
- 安定動作の確認

---

## Phase 7: DOSBox-X環境での統合テストと最適化

### 目的
実環境での動作確認とパフォーマンス最適化

### タスク

#### 7.1 統合テスト
テストシナリオ:
1. 基本的なGETリクエスト
2. POSTリクエスト
3. 大きなファイルのダウンロード
4. エラーケース（存在しないホスト、404エラー等）
5. 複数の連続リクエスト

#### 7.2 パフォーマンス最適化
- バッファサイズの調整
- 不要なメモリコピーの削減
- コンパイラ最適化オプションの調整

#### 7.3 メモリ使用量の最適化
- Small memory modelの制約（64KB）内での動作
- 必要に応じてCompact/Large modelへの移行検討

#### 7.4 ドキュメント整備
- [`README.md`](doscurl/README.md): プロジェクト概要
- [`USAGE.md`](doscurl/docs/USAGE.md): 使用方法
- [`BUILD.md`](doscurl/docs/BUILD.md): ビルド手順
- コード内コメントの充実

#### 7.5 リリース準備
- バージョン番号の設定
- 実行ファイルのパッケージング
- サンプルスクリプトの作成

### テスト環境
```
DOSBox-X設定:
- NE2000エミュレーション有効
- mTCP設定済み
- パケットドライバロード済み
- DNS設定済み
```

### 成果物
- 完全に動作する `doscurl.exe`
- 完全なドキュメント
- テストスクリプト一式

---

## 技術的考慮事項

### メモリモデル
- **Small Model (-ms)**: コードとデータが各64KB以内
- 大規模化した場合は **Compact Model (-mc)** または **Large Model (-ml)** に移行

### WATTCPの制約
- パケットドライバが必要
- DOS環境でのみ動作
- 同時接続数に制限あり

### HTTPプロトコル
- HTTP/1.0を基本とする（シンプル）
- HTTP/1.1の一部機能（Host ヘッダー、chunked encoding）をサポート
- Keep-Aliveは非対応（Connection: close）

### セキュリティ
- HTTPSは非対応（SSL/TLSライブラリが16bit DOSで利用困難）
- 平文通信のみ

---

## 開発スケジュール（目安）

| Phase | 内容 | 推定工数 |
|-------|------|----------|
| Phase 1 | プロジェクト構造 | 1-2日 |
| Phase 2 | ネットワーク接続 | 2-3日 |
| Phase 3 | HTTP GET | 2-3日 |
| Phase 4 | HTTP POST | 1-2日 |
| Phase 5 | 引数パーサー | 1-2日 |
| Phase 6 | エラーハンドリング | 2-3日 |
| Phase 7 | テストと最適化 | 2-3日 |
| **合計** | | **11-18日** |

---

## 参考リソース

### ドキュメント
- [WATTCP Documentation](http://www.wattcp.com/)
- [HTTP/1.0 RFC 1945](https://tools.ietf.org/html/rfc1945)
- [HTTP/1.1 RFC 2616](https://tools.ietf.org/html/rfc2616)
- [Open Watcom C/C++ User's Guide](http://www.openwatcom.org/doc.php)

### 参考実装
- mTCP (http://www.brutman.com/mTCP/)
- curl (https://github.com/curl/curl) - 機能参考用

### テストサイト
- http://httpbin.org/ - HTTPテスト用API
- http://example.com/ - シンプルなテストページ

---

## 次のステップ

このプランを確認後、Phase 1から順次実装を開始します。各Phaseの完了後、次のPhaseに進む前に動作確認とレビューを行います。