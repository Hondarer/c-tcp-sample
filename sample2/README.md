# sample2

```plantuml
@startuml sample2 のシーケンス
caption sample2 のシーケンス (RST送信による強制切断)

participant "クライアント2" as C
participant "サーバー2" as S

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
C -> C: read()

note over S: SO_LINGER設定\n(l_onoff=1, l_linger=0)
S -> C: close() → **RST**
note over C: 強制切断を検知
C -> C: close()

@enduml
```

## 説明

sample2では、サーバー側が**RST（リセット）パケット**による強制切断を行います。

### 通常のclose（sample1）との違い

- **sample1（正規のclose）**: 4-wayハンドシェイク（FIN → ACK → FIN → ACK）で丁寧に接続を終了
- **sample2（RST送信）**: サーバーが一方的にRSTパケットを送信して接続を強制終了

### RST送信の実装

サーバー側で`SO_LINGER`ソケットオプションを以下のように設定：

```c
struct linger so_linger;
so_linger.l_onoff = 1;   // lingerを有効化
so_linger.l_linger = 0;  // タイムアウト0秒 = RST送信
setsockopt(client_fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
close(client_fd);
```

### RSTパケットの特徴

- 未送信データを破棄
- TIME_WAIT状態をスキップ
- 接続を即座に終了
- クライアント側でエラー（Connection reset by peer）が発生する可能性
