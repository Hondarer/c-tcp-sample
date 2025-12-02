# TCPDUMP調査ガイド

このガイドでは、sample1〜3のTCP通信をtcpdumpで調査する方法を説明します。

## 目次

1. [基本的な使い方](#基本的な使い方)
2. [サンプル別の調査方法](#サンプル別の調査方法)
3. [パケットの読み方](#パケットの読み方)
4. [よくあるパターン](#よくあるパターン)

---

## 基本的な使い方

### 最小限のコマンド

```bash
sudo tcpdump -i lo -nn 'port 8081'
```

**オプション説明:**
- `-i lo`: ループバックインターフェース（127.0.0.1）を監視
- `-nn`: ホスト名とポート名の解決を無効化（数値で表示）
- `'port 8081'`: ポート8081の通信のみをキャプチャ

### より詳細な情報を表示

```bash
sudo tcpdump -i lo -nn -v 'port 8081'
```

- `-v`: 詳細表示（-vv, -vvvでさらに詳細）

### パケットの内容を16進数で表示

```bash
sudo tcpdump -i lo -nn -X 'port 8081'
```

- `-X`: パケットの内容を16進数とASCIIで表示

### タイムスタンプをより読みやすく

```bash
sudo tcpdump -i lo -nn -tttt 'port 8081'
```

- `-tttt`: 読みやすいタイムスタンプ形式

### ファイルに保存して後で分析

```bash
# キャプチャを保存
sudo tcpdump -i lo -nn -w capture.pcap 'port 8081'

# 保存したファイルを読み込み
sudo tcpdump -nn -r capture.pcap
```

---

## サンプル別の調査方法

### Sample1: 正規の4-wayハンドシェイク

**ポート:** 8081

```bash
sudo tcpdump -i lo -nn -v 'port 8081'
```

**確認ポイント:**
1. 3-way handshake（SYN → SYN-ACK → ACK）
2. データ送信（ABCD → CDEF）
3. **4-way handshake**（FIN → ACK → FIN → ACK）

**期待される出力:**
```
# 3-way handshake
127.0.0.1.xxxxx > 127.0.0.1.8081: Flags [S]      # SYN
127.0.0.1.8081 > 127.0.0.1.xxxxx: Flags [S.]     # SYN-ACK
127.0.0.1.xxxxx > 127.0.0.1.8081: Flags [.]      # ACK

# データ送信
127.0.0.1.xxxxx > 127.0.0.1.8081: Flags [P.]     # PSH+ACK (ABCD)
127.0.0.1.8081 > 127.0.0.1.xxxxx: Flags [.]      # ACK
127.0.0.1.8081 > 127.0.0.1.xxxxx: Flags [P.]     # PSH+ACK (CDEF)
127.0.0.1.xxxxx > 127.0.0.1.8081: Flags [.]      # ACK

# 正規のclose (4-way handshake)
127.0.0.1.xxxxx > 127.0.0.1.8081: Flags [F.]     # FIN+ACK
127.0.0.1.8081 > 127.0.0.1.xxxxx: Flags [.]      # ACK
127.0.0.1.8081 > 127.0.0.1.xxxxx: Flags [F.]     # FIN+ACK
127.0.0.1.xxxxx > 127.0.0.1.8081: Flags [.]      # ACK
```

---

### Sample2: RST送信による強制切断

**ポート:** 8082

```bash
sudo tcpdump -i lo -nn -v 'port 8082'
```

**確認ポイント:**
1. 3-way handshake
2. データ送信（ABCD → CDEF）
3. **RSTパケット**（FINではなくRST）

**期待される出力:**
```
# 3-way handshake
127.0.0.1.xxxxx > 127.0.0.1.8082: Flags [S]      # SYN
127.0.0.1.8082 > 127.0.0.1.xxxxx: Flags [S.]     # SYN-ACK
127.0.0.1.xxxxx > 127.0.0.1.8082: Flags [.]      # ACK

# データ送信
127.0.0.1.xxxxx > 127.0.0.1.8082: Flags [P.]     # PSH+ACK (ABCD)
127.0.0.1.8082 > 127.0.0.1.xxxxx: Flags [.]      # ACK
127.0.0.1.8082 > 127.0.0.1.xxxxx: Flags [P.]     # PSH+ACK (CDEF)
127.0.0.1.xxxxx > 127.0.0.1.8082: Flags [.]      # ACK

# RST送信 (強制切断)
127.0.0.1.8082 > 127.0.0.1.xxxxx: Flags [R.]     # RST+ACK
# ここでクライアントがFINを送ろうとするかもしれない
127.0.0.1.xxxxx > 127.0.0.1.8082: Flags [F.]     # FIN+ACK
127.0.0.1.8082 > 127.0.0.1.xxxxx: Flags [R]      # RST (接続がすでにない)
```

**重要:** RSTパケットが送信されると、それ以降のFINやACKに対してもRSTで応答する

---

### Sample3: 読み取り遅延時のRST動作

**ポート:** 8083

```bash
sudo tcpdump -i lo -nn -tttt -v 'port 8083'
```

**確認ポイント:**
1. データ送信後、クライアントが3秒待機
2. **その間にサーバーがRSTを送信**
3. クライアントの読み取り試行とRSTの関係

**期待される出力:**
```
# 3-way handshake
HH:MM:SS.xxxxxx 127.0.0.1.xxxxx > 127.0.0.1.8083: Flags [S]
HH:MM:SS.xxxxxx 127.0.0.1.8083 > 127.0.0.1.xxxxx: Flags [S.]
HH:MM:SS.xxxxxx 127.0.0.1.xxxxx > 127.0.0.1.8083: Flags [.]

# データ送信
HH:MM:SS.xxxxxx 127.0.0.1.xxxxx > 127.0.0.1.8083: Flags [P.] # ABCD
HH:MM:SS.xxxxxx 127.0.0.1.8083 > 127.0.0.1.xxxxx: Flags [.]
HH:MM:SS.xxxxxx 127.0.0.1.8083 > 127.0.0.1.xxxxx: Flags [P.] # CDEF
HH:MM:SS.xxxxxx 127.0.0.1.xxxxx > 127.0.0.1.8083: Flags [.]

# サーバーがRSTを送信 (クライアントはsleep中)
HH:MM:SS.xxxxxx 127.0.0.1.8083 > 127.0.0.1.xxxxx: Flags [R.]

# 3秒後、クライアントが目覚める (タイムスタンプの差に注目)
# クライアントがFINを送ろうとする
HH:MM:SS+3.xxxxxx 127.0.0.1.xxxxx > 127.0.0.1.8083: Flags [F.]
HH:MM:SS+3.xxxxxx 127.0.0.1.8083 > 127.0.0.1.xxxxx: Flags [R]
```

**重要:** タイムスタンプの差で、RSTがクライアントの読み取り前に送信されたことが確認できる

---

## パケットの読み方

### TCPフラグの意味

| フラグ | 記号 | 意味 |
|--------|------|------|
| SYN | S | 接続開始 |
| ACK | . | 確認応答 |
| FIN | F | 正規の接続終了 |
| RST | R | 強制切断（リセット） |
| PSH | P | データの即座の転送 |

### フラグの組み合わせ

- `[S]`: SYNのみ（接続要求）
- `[S.]`: SYN+ACK（接続承認）
- `[.]`: ACKのみ（確認）
- `[P.]`: PSH+ACK（データ送信＋確認）
- `[F.]`: FIN+ACK（終了要求＋確認）
- `[R.]`: RST+ACK（強制切断＋確認）
- `[R]`: RSTのみ（強制切断）

### パケット情報の読み方

```
127.0.0.1.45678 > 127.0.0.1.8081: Flags [S], seq 1234567890, win 65535, length 0
```

- `127.0.0.1.45678`: 送信元IPとポート（クライアント）
- `127.0.0.1.8081`: 宛先IPとポート（サーバー）
- `Flags [S]`: TCPフラグ（SYN）
- `seq 1234567890`: シーケンス番号
- `win 65535`: ウィンドウサイズ（受信バッファサイズ）
- `length 0`: データ長（バイト）

---

## よくあるパターン

### 3-way handshake（接続確立）

```
Client → Server: [S]      # クライアントが接続要求
Server → Client: [S.]     # サーバーが承認
Client → Server: [.]      # クライアントが確認
```

### 4-way handshake（正規の切断）

```
Client → Server: [F.]     # クライアントが終了要求
Server → Client: [.]      # サーバーが確認
Server → Client: [F.]     # サーバーも終了要求
Client → Server: [.]      # クライアントが確認
```

### RST（強制切断）

```
Server → Client: [R.]     # サーバーが強制切断
# または
Server → Client: [R]      # ACKなしのRST
```

**RSTの特徴:**
- 一方的な切断（確認応答なし）
- 受信バッファのデータが破棄される可能性
- TIME_WAIT状態をスキップ
- 相手からのパケットに対してもRSTで応答

---

## 実践的なデバッグ手順

### 1. 両サンプルを比較する

```bash
# ターミナル1: sample1のキャプチャ
sudo tcpdump -i lo -nn -v 'port 8081' > sample1.log

# ターミナル2: sample2のキャプチャ
sudo tcpdump -i lo -nn -v 'port 8082' > sample2.log

# 後でdiffで比較
diff sample1.log sample2.log
```

### 2. タイミングを詳細に確認

```bash
# タイムスタンプとシーケンス番号を表示
sudo tcpdump -i lo -nn -tttt -S 'port 8083'
```

- `-S`: 相対シーケンス番号ではなく絶対シーケンス番号を表示

### 3. データの内容を確認

```bash
# 16進数ダンプでABCDとCDEFを確認
sudo tcpdump -i lo -nn -X 'port 8081'
```

**出力例:**
```
0x0000:  4142 4344                                ABCD
0x0000:  4344 4546                                CDEF
```

### 4. 複数ポートを同時監視

```bash
# 全サンプルを同時にキャプチャ
sudo tcpdump -i lo -nn -v 'port 8081 or port 8082 or port 8083'
```

---

## トラブルシューティング

### パケットが見えない場合

1. **ループバックインターフェースを確認**
   ```bash
   ip addr show lo
   ```

2. **sudoで実行しているか確認**
   - tcpdumpは管理者権限が必要

3. **正しいインターフェースを指定しているか確認**
   ```bash
   # 全インターフェースを表示
   tcpdump -D
   ```

### パケットが多すぎる場合

```bash
# パケット数を制限
sudo tcpdump -i lo -nn -c 50 'port 8081'
```

- `-c 50`: 50パケットでキャプチャを停止

---

## まとめ

### Sample1 vs Sample2 vs Sample3

| サンプル | 切断方法 | 確認ポイント |
|----------|----------|--------------|
| Sample1 | 正規のFIN | 4-way handshake（FIN → ACK → FIN → ACK） |
| Sample2 | RST送信 | RSTパケット、その後のパケットもRSTで応答 |
| Sample3 | RST送信（遅延） | RSTとクライアント読み取りのタイミング |

### 覚えておくべきコマンド

```bash
# 基本
sudo tcpdump -i lo -nn -v 'port 8081'

# タイムスタンプ付き
sudo tcpdump -i lo -nn -tttt 'port 8082'

# データ内容確認
sudo tcpdump -i lo -nn -X 'port 8083'

# 全ポート監視
sudo tcpdump -i lo -nn -v 'port 8081 or port 8082 or port 8083'
```

これらのツールを使って、TCP通信の詳細な挙動を理解しましょう！
