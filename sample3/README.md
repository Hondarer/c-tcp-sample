# sample3

```plantuml
@startuml sample3 のシーケンス
caption sample3 のシーケンス (読み取り遅延時のRST動作検証)

participant "クライアント3" as C
participant "サーバー3" as S

S -> S: socket()
S -> S: bind()
S -> S: listen()
S -> S: accept() (待機)

C -> C: socket()
C -> S: connect() → SYN
S -> C: SYN-ACK
C -> S: ACK
S -> S: accept() 完了

C -> S: write("ABCD") → データ
S -> S: read()
S -> C: write("CDEF") → データ

note over S: SO_LINGER設定\n(l_onoff=1, l_linger=0)
S -> C: close() → **RST**

note over C: 3秒間sleep中\nまだデータを読んでいない
C -> C: sleep(3)

note over C: sleep終了後\nselect()を呼び出し
C -> C: select()
note over C: RSTを受信済み\nソケットはエラー状態

C -> C: read()
note over C: read結果:\n- 0が返る (EOF)\n- または-1でエラー

C -> C: close()

@enduml
```

## 検証目的

**クライアントが読み取りを遅延させた場合、サーバーのRST送信がどう影響するかを検証**

### 検証シナリオ

1. クライアントが "ABCD" を送信
2. サーバーが "CDEF" を送信（受信バッファに格納される）
3. **サーバーがRSTパケットを送信して強制切断**
4. クライアントは3秒間sleep（まだデータを読み取っていない）
5. sleep後、クライアントがselect()とread()を呼び出す

### 期待される動作

#### RSTパケットの影響

- **RSTパケット受信時**: TCPスタックはソケットをエラー状態にする
- **受信バッファのデータ**: RSTにより破棄される可能性がある（実装依存）

#### クライアント側の挙動

以下のいずれかの動作が想定される：

**パターン1: データが読める場合**
```
送信: ABCD
3秒待機中...
読み取り開始
受信: CDEF
```
- 受信バッファのデータがRST前に到着していれば読める可能性がある
- その後のread()でエラーまたはEOFを検知

**パターン2: 即座にエラー**
```
送信: ABCD
3秒待機中...
読み取り開始
接続が閉じられました
```
または
```
read failed: Connection reset by peer
```
- RSTによりソケットがエラー状態になり、データが読めない
- read()が0 (EOF)または-1 (エラー)を返す

### sample2との違い

- **sample2**: クライアントがすぐに読み取りを試みる
  - データを読める可能性が高い

- **sample3**: クライアントが3秒遅延してから読み取りを試みる
  - RSTの影響を受けやすい
  - 受信バッファのデータが破棄される可能性がある

### 実行方法

```bash
# ビルド
make

# ターミナル1でサーバー起動
./server3

# ターミナル2でクライアント実行
./client3
```

### tcpdumpでの確認

```bash
sudo tcpdump -i lo -nn 'port 8083'
```

RSTパケットのタイミングとクライアントの読み取りタイミングを確認できる。

### 重要な知見

この実験により、以下を理解できる：

1. **RSTパケットの破壊力**: 正規のFINと違い、RSTは受信バッファのデータを破棄する可能性がある
2. **タイミングの重要性**: クライアントの読み取りタイミングによって結果が変わる
3. **実装依存性**: OSやTCPスタックの実装により動作が異なる可能性がある
