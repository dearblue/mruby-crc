class CRC
  def CRC.[](*args)
    if configured?
      crc = new(*args[1 .. -1])
      crc << args[0] if args[0]
      crc
    else
      new(*args)
    end
  end

  def CRC.===(crc2)
    if bitsize == crc2.bitsize &&
       polynomial == crc2.polynomial &&
       reflect_input? == crc2.reflect_input? &&
       reflect_output? == crc2.reflect_output? &&
       xor_external == xor_external
      true
    else
      false
    end
  rescue # e.g. no method error
    nil
  end

  def CRC.digest(seq, prev_crc = nil)
    crc = self[seq, prev_crc]
    crc.digest
  end

  def CRC.hexdigest(seq, prev_crc = nil)
    crc = self[seq, prev_crc]
    crc.hexdigest
  end

  def CRC.crc(seq, prev_crc = nil)
    crc = self[seq, prev_crc]
    crc.finish
  end

  def inspect
    "#<#{self.class}:0x#{hexdigest}>"
  end

  def pretty_inspect(q)
    q.text inspect
  end

  def combine(crc2)
    g1 = self.class
    g2 = crc2.class
    if g1 === g2
      g1.combine crc2.crc, crc2.size
    else
      raise 
    end
  end
end

class CRC
  define_crc_module("CRC1",                1,               "01",          0, true,  true,    ~0)
  define_crc_module("CRC3_ROHC",           3,               "03",         ~0, true,  true,     0)
  define_crc_module("CRC4_INTERLAKEN",     4,               "03",          0, false, false,   ~0)
  define_crc_module("CRC4_ITU",            4,               "03",          0, true,  true,     0)
  define_crc_module("CRC5_EPC",            5,               "09",       "09", false, false,    0)
  define_crc_module("CRC5_ITU",            5,               "15",          0, true,  true,     0)
  define_crc_module("CRC5_USB",            5,               "05",          0, true,  true,    ~0)
  define_crc_module("CRC6_CDMA2000_A",     6,               "27",         ~0, false, false,    0)
  define_crc_module("CRC6_CDMA2000_B",     6,               "07",         ~0, false, false,    0)
  define_crc_module("CRC6_DARC",           6,               "19",          0, true,  true,     0)
  define_crc_module("CRC6_ITU",            6,               "03",          0, true,  true,     0)
  define_crc_module("CRC7",                7,               "09",          0, false, false,    0)
  define_crc_module("CRC7_ROHC",           7,               "4f",         ~0, true,  true,     0)
  define_crc_module("CRC7_UMTS",           7,               "45",          0, false, false,    0)
  define_crc_module("CRC8_CCITT",          8,               "07",          0, false, false,    0)
  define_crc_module("CRC8_MAXIM",          8,               "31",          0, true,  true,     0)
  define_crc_module("CRC8_DARC",           8,               "39",          0, true,  true,     0)
  define_crc_module("CRC8_SAE",            8,               "1d",          0, false, false,   ~0)
  define_crc_module("CRC8_WCDMA",          8,               "9b",          0, true,  true,     0)
  define_crc_module("CRC8_CDMA2000",       8,               "9b",         ~0, false, false,    0)
  define_crc_module("CRC8_DVB_S2",         8,               "d5",          0, false, false,    0)
  define_crc_module("CRC8_EBU",            8,               "1d",         ~0, true,  true,     0)
  define_crc_module("CRC8_I_CODE",         8,               "1d",       "fd", false, false,    0)
  define_crc_module("CRC8_ITU",            8,               "07",       "55", false, false, "55")
  define_crc_module("CRC8_LTE",            8,               "9b",          0, false, false,    0)
  define_crc_module("CRC8_ROHC",           8,               "07",         ~0, true,  true,     0)
  define_crc_module("CRC10",              10,             "0233",          0, false, false,    0)
  define_crc_module("CRC10_CDMA2000",     10,             "03d9",         ~0, false, false,    0)
  define_crc_module("CRC11",              11,             "0385",     "001a", false, false,    0)
  define_crc_module("CRC11_UMTS",         11,             "0307",          0, false, false,    0)
  define_crc_module("CRC12_CDMA2000",     12,             "0f13",         ~0, false, false,    0)
  define_crc_module("CRC12_DECT",         12,             "080f",          0, false, false,    0)
  define_crc_module("CRC12_UMTS",         12,             "080f",          0, false, true,     0)
  define_crc_module("CRC13_BBC",          13,             "1cf5",          0, false, false,    0)
  define_crc_module("CRC14_DARC",         14,             "0805",          0, true,  true,     0)
  define_crc_module("CRC15",              15,             "4599",          0, false, false,    0)
  define_crc_module("CRC15_MPT1327",      15,             "6815",          1, false, false,    1)
  define_crc_module("CRC16",              16,             "8005",          0, true,  true,     0)
  define_crc_module("CRC16_AUG_CCITT",    16,             "1021",     "1d0f", false, false,    0)
  define_crc_module("CRC16_CDMA2000",     16,             "c867",         ~0, false, false,    0)
  define_crc_module("CRC16_DECT_R",       16,             "0589",          1, false, false,    1)
  define_crc_module("CRC16_DECT_X",       16,             "0589",          0, false, false,    0)
  define_crc_module("CRC16_T10_DIF",      16,             "8bb7",          0, false, false,    0)
  define_crc_module("CRC16_DNP",          16,             "3d65",         ~0, true,  true,    ~0)
  define_crc_module("CRC16_BUYPASS",      16,             "8005",          0, false, false,    0)
  define_crc_module("CRC16_CCITT_FALSE",  16,             "1021",         ~0, false, false,    0)
  define_crc_module("CRC16_DDS_110",      16,             "8005",     "800d", false, false,    0)
  define_crc_module("CRC16_EN_13757",     16,             "3d65",         ~0, false, false,   ~0)
  define_crc_module("CRC16_GENIBUS",      16,             "1021",          0, false, false,   ~0)
  define_crc_module("CRC16_LJ1200",       16,             "6f63",          0, false, false,    0)
  define_crc_module("CRC16_MAXIM",        16,             "8005",         ~0, true,  true,    ~0)
  define_crc_module("CRC16_MCRF4XX",      16,             "1021",         ~0, true,  true,     0)
  define_crc_module("CRC16_RIELLO",       16,             "1021",     "b2aa", true,  true,     0)
  define_crc_module("CRC16_TELEDISK",     16,             "a097",          0, false, false,    0)
  define_crc_module("CRC16_TMS37157",     16,             "1021",     "89ec", true,  true,     0)
  define_crc_module("CRC16_USB",          16,             "8005",          0, true,  true,    ~0)
  define_crc_module("CRC16_A",            16,             "1021",     "c6c6", true,  true,     0)
  define_crc_module("CRC16_KERMIT",       16,             "1021",          0, true,  true,     0)
  define_crc_module("CRC16_MODBUS",       16,             "8005",         ~0, true,  true,     0)
  define_crc_module("CRC16_X_25",         16,             "1021",          0, true,  true,    ~0)
  define_crc_module("CRC16_XMODEM",       16,             "1021",          0, false, false,    0)
  define_crc_module("CRC24_Radix_64",     24,         "00864cfb",          0, false, false,    0)
  define_crc_module("CRC24_OPENPGP",      24,         "00864cfb", "00b704ce", false, false,    0)
  define_crc_module("CRC24_BLE",          24,         "0000065b", "00555555", true,  true,     0)
  define_crc_module("CRC24_FLEXRAY_A",    24,         "005d6dcb", "00fedcba", false, false,    0)
  define_crc_module("CRC24_FLEXRAY_B",    24,         "005d6dcb", "00abcdef", false, false,    0)
  define_crc_module("CRC24_INTERLAKEN",   24,         "00328b63",          0, false, false,   ~0)
  define_crc_module("CRC24_LTE_A",        24,         "00864cfb",          0, false, false,    0)
  define_crc_module("CRC24_LTE_B",        24,         "00800063",          0, false, false,    0)
  define_crc_module("CRC30_CDMA",         30,         "2030b9c7",          0, false, false,   ~0)
  define_crc_module("CRC31_PHILIPS",      31,         "04c11db7",          0, false, false,   ~0)
  define_crc_module("CRC32",              32,         "04c11db7",          0, true,  true,    ~0)
  define_crc_module("CRC32_BZIP2",        32,         "04c11db7",          0, false, false,   ~0)
  define_crc_module("CRC32C",             32,         "1edc6f41",          0, true,  true,    ~0)
  define_crc_module("CRC32D",             32,         "a833982b",          0, true,  true,    ~0)
  define_crc_module("CRC32_MPEG_2",       32,         "04c11db7",         ~0, false, false,    0)
  define_crc_module("CRC32_POSIX",        32,         "04c11db7",         ~0, false, false,   ~0)
  define_crc_module("CRC32Q",             32,         "814141ab",          0, false, false,    0)
  define_crc_module("CRC32_JAMCRC",       32,         "04c11db7",         ~0, true,  true,     0)
  define_crc_module("CRC32_XFER",         32,         "000000af",          0, false, false,    0)
  define_crc_module("CRC40_GSM",          40, "0000000004820009",         ~0, false, false,   ~0)
  define_crc_module("CRC64",              64, "42f0e1eba9ea3693",          0, true,  true,    ~0)
  define_crc_module("CRC64_JONES",        64, "ad93d23594c935a9",         ~0, true,  true,     0)
  define_crc_module("CRC64_ECMA",         64, "42f0e1eba9ea3693",          0, false, false,    0)
  define_crc_module("CRC64_WE",           64, "42f0e1eba9ea3693",          0, false, false,   ~0)
  define_crc_module("CRC64_ISO",          64, "000000000000001b",          0, true,  true,     0)
end
