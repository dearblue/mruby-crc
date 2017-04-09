# mruby-crc - A general CRC calcurator for mruby

利用者が定義可能な汎用 CRC 生成器です。

動的に定義可能な項目は次の通りです:

  * ビット長: 1 〜 64 ビット
  * 生成多項式: 任意の整数値
  * CRC の初期値: 任意の整数値
  * 入力ビットの順序: 順送り・逆送り (normal input / reflect input)
  * 出力ビットの順序: 順送り・逆送り (normal output / reflect output)
  * 出力の XOR 値: 任意の整数値
  * 計算アルゴリズム: bit-by-bit, bit-by-bit-fast, halfbyte-table, standard-table, slicing-by-4/8/16


## 必要なもの

  * ``C99`` 対応コンパイラ
  * ``mruby-1.2`` または最新版 (?)


## ``MRB_INT16``、``MRB_INT64`` が定義された場合の挙動の変化について

``MRB_INT64`` が定義された場合は、``CRC#finish`` などの値が常に整数値となります。

``MRB_INT64`` が未定義の場合は、``MRB_INT_MAX`` を越えた場合は ``CRC#finish`` などの値が hexdigest された文字列となります。

定義・未定義にかかわらず、``CRC.new`` メソッドの引数 ``polynomial`` ``initial_crc`` ``xor_output`` には hexdigest されたものと同等の文字列を与えることが出来ます。

``` ruby
CRC32C = CRC.new(32, "1edc6f41", 0, true, true, ~0)

p CRC32C.crc("123456789")
## => 3808858755 (MRB_INT64 が定義された場合)
## => "e3069283" (MRB_INT64 が未定義の場合)

p CRC32C.crc("123456789" * 6)
## => 1521416889 (MRB_INT16 が未定義の場合)
## => "5aaefab9" (MRB_INT16 が定義された場合)
```

## 簡易 API 案内

  * CRC.new(bitsize, polynomial, initial\_crc = 0, refin = true, refout = true, xor\_output = ~0, algorithm = CRC::STANDARD\_TABLE) -> CRC generator class
  * CRC.new(continuous\_crc = 0, input\_length = 0) -> crc generator object
  * CRC[seq, continuous\_crc = 0, input\_length = 0] -> crc generator object
  * crc.update(seq) -> self
  * crc.reset(continuous\_crc = 0, input\_length = 0) -> self
  * crc.finish -> crc integer
  * crc.digest -> binary digested string (liked as Digest#digest)
  * crc.hexdigest -> hexa-decimal digested string (liked as Digest#hexdigest)


## 既定で使われるアルゴリズムの変更

``build_config.rb`` で ``conf.cc.defines`` に追加することで、``CRC.new`` のアルゴリズム指定を省略した場合の挙動を変更できます。
何も指定しない場合は、``CRC_DEFAULT_STANDARD_TABLE`` が指定されたものとして扱われます。

  * ``CRC_DEFAULT_BITBYBIT``
  * ``CRC_DEFAULT_BITBYBIT_FAST``
  * ``CRC_DEFAULT_HALFBYTE_TABLE``
  * ``CRC_DEFAULT_STANDARD_TABLE``
  * ``CRC_DEFAULT_SLICING_BY_4``
  * ``CRC_DEFAULT_SLICING_BY_8``
  * ``CRC_DEFAULT_SLICING_BY_16``

``build_config.rb`` の例:

```ruby
MRuby::Build.new("host") do |conf|
  .....
  conf.cc.defines << "CRC_DEFAULT_SLICING_BY_16"
  .....
end
```

## 内部処理の整数値型の固定


mruby-crc は CRC のビット長によって内部の整数値型を調整しています。具体的には、``uint8_t`` ``uint16_t`` ``uint32_t`` ``uint64_t`` のためにそれぞれの関数を生成するようになっています。

この mruby-crc 内部で使われる整数値型を固定して僅かでもバイナリサイズを少なくしたい場合、build\_config.rb 内で ``CRC_ONLY_INT***`` を定義します。

定義名は ``CRC_ONLY_INT8`` ``CRC_ONLY_INT16`` ``CRC_ONLY_INT32`` ``CRC_ONLY_INT64`` のみが有効です。

```ruby:build_config.rb
MRuby::Build.new("host") do |conf|
  .....
  conf.cc.defines << "CRC_ONLY_INT32"
  .....
end
```

整数値レジスタが32ビット長であるプロセッサの場合、``CRC_ONLY_INT64`` を定義すると CRC-32 の計算であっても ``uint64_t`` による計算となり、計算速度が数倍遅くなります。

また、``CRC_ONLY_INT8`` を定義した場合、8ビット長を超える CRC の定義は出来なくなります。


## ルックアップテーブルのメモリ確保量

CRC を計算する時に高速化するため、入力バイト値に対する変化を予め求めておくわけですが、この際に専有するメモリの量は algorithm の値によって決定されます。

このテーブルの確保・初期化が行われる時機は、CRC を最初に計算する直前となります。
CRC.new した段階ではメモリの確保も初期化も行われません。

<table>
  <tr><th>algorithm           <th>メモリ使用量の求め方 (CRC-32 の場合)  <th>確保されるメモリバイト数 (CRC-32 の場合)
  <tr><td>CRC::BITBYBIT       <td>(N/A)                                 <td>0 bytes
  <tr><td>CRC::BITBYBIT_FAST  <td>(N/A)                                 <td>0 bytes
  <tr><td>CRC::HALFBYTE_TABLE <td>sizeof(uint32_t[16])                  <td>64 bytes
  <tr><td>CRC::STANDARD_TABLE <td>sizeof(uint32_t[256])                 <td>1024 bytes (1 KiB)
  <tr><td>CRC::SLICING_BY_4   <td>sizeof(uint32_t[4][256])              <td>4096 bytes (4 KiB)
  <tr><td>CRC::SLICING_BY_8   <td>sizeof(uint32_t[8][256])              <td>8192 bytes (8 KiB)
  <tr><td>CRC::SLICING_BY_16  <td>sizeof(uint32_t[16][256])             <td>16384 bytes (16 KiB)
</table>
