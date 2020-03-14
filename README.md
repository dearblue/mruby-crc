# mruby-crc - A general CRC calcurator for mruby

利用者が定義可能な汎用 CRC 生成器です。

動的に定義可能な項目は次の通りです:

  * ビット長: 1 〜 64 ビット
  * 生成多項式: 任意の整数値
  * CRC の初期値: 任意の整数値
  * 入力ビットの順序: 順送り・逆送り (normal input / reflect input)
  * 出力ビットの順序: 順送り・逆送り (normal output / reflect output)
  * 入力の位置: 入力の前にゼロを詰める・後ろにゼロを詰める (prepend zero / append zero)
  * 出力の XOR 値: 任意の整数値
  * 計算アルゴリズム (ビルド時に固定): bitwise, bitcombine, lookup table


## 必要なもの

  * ``C99`` 対応コンパイラ
  * ``mruby-1.2`` または最新版 (?)


## `Integer` (`mrb_int`) を超える数値の表現について

`mrb_int` の範囲外の数値 (`0..MRB_INT_MAX`) は、`CRC#finish` などの値が `hexdigest` された文字列オブジェクトとなります。

定義・未定義にかかわらず、``CRC.new`` メソッドの引数 ``polynomial`` ``initial_crc`` ``xor_output`` には hexdigest されたものと同等の文字列を与えることが出来ます。

``` ruby
CRC32C = CRC.new(32, "1edc6f41", 0, true, true, ~0)

p CRC32C.crc("123456789")
## => 3808858755 (MRB_INT64 が定義された場合)
## => "e3069283" (MRB_INT32 が未定義の場合)
```

## 簡易 API 案内

  * CRC.new(bitsize, polynomial, initialcrc = 0, refin = true, refout = true, xoroutput = ~0, appendzero = true) -> CRC calcurator class
  * CRC.new(bitsize, polynomial, initialcrc: 0, reflectin: true, reflectout: true, xoroutput: ~0, appendzero: true) -> CRC calcurator class
  * CRC.new(previous_crc = 0, input_length = 0) -> crc calcurator object
  * CRC[seq, previous_crc = 0, input_length = 0] -> crc calcurator object
  * crc.update(seq) -> self
  * crc.reset(previous_crc = 0, input_length = 0) -> self
  * crc.finish -> crc integer
  * crc.digest -> binary digested string (liked as Digest#digest)
  * crc.hexdigest -> hexa-decimal digested string (liked as Digest#hexdigest)


## 使われるアルゴリズムの変更

``build_config.rb`` で ``conf.cc.defines`` に追加することで、CRC の計算で使用するアルゴリズムを変更することが出来ます。
何も指定しない場合は、``CRCEA_BY4_OCTET`` (Slicing by 4 相当) が指定されたものとして扱われます。

``build_config.rb`` の例:

```ruby
MRuby::Build.new("host") do |conf|
  .....

  # オブジェクトコードを最小限に抑える場合
  conf.cc.defines << %w(CRCEA_MINIMAL CRCEA_ONLY_INT64)

  # メモリを使っても高速にする場合
  #conf.cc.defines << %w(CRCEA_SMALL CRCEA_ENABLE_BY16_OCTET CRCEA_DEFAULT_ALGORITHM=CRCEA_BY16_OCTET)

  .....
end
```

注: mruby から動的にアルゴリズムの指定を行うことは出来なくなりました。


## 内部処理の整数値型の固定


mruby-crc が使っている libcrcea は CRC のビット長によって内部の整数値型を調整しています。具体的には、``uint8_t`` ``uint16_t`` ``uint32_t`` ``uint64_t`` のためにそれぞれの関数を生成するようになっています。

この整数値型を固定して僅かでもバイナリサイズを少なくしたい場合、build\_config.rb 内で ``CRCEA_ONLY_INT***`` を定義します。

定義名は ``CRCEA_ONLY_INT8`` ``CRCEA_ONLY_INT16`` ``CRC_ONLY_INT32`` ``CRCEA_ONLY_INT64`` のみが有効です。

```ruby:build_config.rb
MRuby::Build.new("host") do |conf|
  .....
  conf.cc.defines << "CRCEA_ONLY_INT32"
  .....
end
```

整数値レジスタが32ビット長であるプロセッサの場合、``CRCEA_ONLY_INT64`` を定義すると CRC-32 の計算であっても ``uint64_t`` による計算となり、計算速度が数倍遅くなります。

また、``CRCEA_ONLY_INT8`` を定義した場合、8ビット長を超える CRC の定義は出来なくなります。


## ルックアップテーブルのメモリ確保量

CRC を計算する時に高速化するため、入力バイト値に対する変化を予め求めておくわけですが、この際に専有するメモリの量は algorithm の値によって決定されます。

このテーブルの確保・初期化が行われる時機は、CRC を最初に計算する直前となります。
CRC.new した段階ではメモリの確保も初期化も行われません。

| algorithm (一部のみ) | メモリ使用量の求め方 (CRC-32 の場合) | 確保されるメモリバイト数 (CRC-32 の場合) |
| -------------------- | ------------------------------------ | ---------------------------------------- |
| `CRCEA_BITWISE*`     | (N/A)                                | 0 bytes                                  |
| `CRCEA_FALLBACK`     | (N/A)                                | 0 bytes                                  |
| `CRCEA_BY4_QUARTET`  | `sizeof(uint32_t[8][16])`            | 512 bytes                                |
| `CRCEA_BY4_OCTET`    | `sizeof(uint32_t[4][256])`           | 4096 bytes (4 KiB)                       |


## Specification

  * Product name: mruby-crc
  * Version: 0.3.2
  * Product quality: PROTOTYPE
  * Author: [dearblue](https://github.com/dearblue)
  * Project page: <https://github.com/dearblue/mruby-crc>
  * Licensing: [2 clause BSD License](LICENSE)
  * Dependency external mrbgems: (NONE)
  * Bundled C libraries: (NONE)
