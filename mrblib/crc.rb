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
  CRC1              = new( 1,               "01",                  0, true,  true,                  ~0)
  CRC3_ROHC         = new( 3,               "03",                 ~0, true,  true,                   0)
  CRC4_INTERLAKEN   = new( 4,               "03",                  0, false, false,                 ~0)
  CRC4_ITU          = new( 4,               "03",                  0, true,  true,                   0)
  CRC5_EPC          = new( 5,               "09",               "09", false, false,                  0)
  CRC5_ITU          = new( 5,               "15",                  0, true,  true,                   0)
  CRC5_USB          = new( 5,               "05",                  0, true,  true,                  ~0)
  CRC6_CDMA2000_A   = new( 6,               "27",                 ~0, false, false,                  0)
  CRC6_CDMA2000_B   = new( 6,               "07",                 ~0, false, false,                  0)
  CRC6_DARC         = new( 6,               "19",                  0, true,  true,                   0)
  CRC6_ITU          = new( 6,               "03",                  0, true,  true,                   0)
  CRC7              = new( 7,               "09",                  0, false, false,                  0)
  CRC7_ROHC         = new( 7,               "4f",                 ~0, true,  true,                   0)
  CRC7_UMTS         = new( 7,               "45",                  0, false, false,                  0)
  CRC8_CCITT        = new( 8,               "07",                  0, false, false,                  0)
  CRC8_MAXIM        = new( 8,               "31",                  0, true,  true,                   0)
  CRC8_DARC         = new( 8,               "39",                  0, true,  true,                   0)
  CRC8_SAE          = new( 8,               "1d",                  0, false, false,                 ~0)
  CRC8_WCDMA        = new( 8,               "9b",                  0, true,  true,                   0)
  CRC8_CDMA2000     = new( 8,               "9b",                 ~0, false, false,                  0)
  CRC8_DVB_S2       = new( 8,               "d5",                  0, false, false,                  0)
  CRC8_EBU          = new( 8,               "1d",                 ~0, true,  true,                   0)
  CRC8_I_CODE       = new( 8,               "1d",               "fd", false, false,                  0)
  CRC8_ITU          = new( 8,               "07",               "55", false, false,               "55")
  CRC8_LTE          = new( 8,               "9b",                  0, false, false,                  0)
  CRC8_ROHC         = new( 8,               "07",                 ~0, true,  true,                   0)
  CRC10             = new(10,             "0233",                  0, false, false,                  0)
  CRC10_CDMA2000    = new(10,             "03d9",                 ~0, false, false,                  0)
  CRC11             = new(11,             "0385",             "001a", false, false,                  0)
  CRC11_UMTS        = new(11,             "0307",                  0, false, false,                  0)
  CRC12_CDMA2000    = new(12,             "0f13",                 ~0, false, false,                  0)
  CRC12_DECT        = new(12,             "080f",                  0, false, false,                  0)
  CRC12_UMTS        = new(12,             "080f",                  0, false, true,                   0)
  CRC13_BBC         = new(13,             "1cf5",                  0, false, false,                  0)
  CRC14_DARC        = new(14,             "0805",                  0, true,  true,                   0)
  CRC15             = new(15,             "4599",                  0, false, false,                  0)
  CRC15_MPT1327     = new(15,             "6815",                  1, false, false,                  1)
  CRC16             = new(16,             "8005",                  0, true,  true,                   0)
  CRC16_AUG_CCITT   = new(16,             "1021",             "1d0f", false, false,                  0)
  CRC16_CDMA2000    = new(16,             "c867",                 ~0, false, false,                  0)
  CRC16_DECT_R      = new(16,             "0589",                  1, false, false,                  1)
  CRC16_DECT_X      = new(16,             "0589",                  0, false, false,                  0)
  CRC16_T10_DIF     = new(16,             "8bb7",                  0, false, false,                  0)
  CRC16_DNP         = new(16,             "3d65",                 ~0, true,  true,                  ~0)
  CRC16_BUYPASS     = new(16,             "8005",                  0, false, false,                  0)
  CRC16_CCITT_FALSE = new(16,             "1021",                 ~0, false, false,                  0)
  CRC16_DDS_110     = new(16,             "8005",             "800d", false, false,                  0)
  CRC16_EN_13757    = new(16,             "3d65",                 ~0, false, false,                 ~0)
  CRC16_GENIBUS     = new(16,             "1021",                  0, false, false,                 ~0)
  CRC16_LJ1200      = new(16,             "6f63",                  0, false, false,                  0)
  CRC16_MAXIM       = new(16,             "8005",                 ~0, true,  true,                  ~0)
  CRC16_MCRF4XX     = new(16,             "1021",                 ~0, true,  true,                   0)
  CRC16_RIELLO      = new(16,             "1021",             "b2aa", true,  true,                   0)
  CRC16_TELEDISK    = new(16,             "a097",                  0, false, false,                  0)
  CRC16_TMS37157    = new(16,             "1021",             "89ec", true,  true,                   0)
  CRC16_USB         = new(16,             "8005",                  0, true,  true,                  ~0)
  CRC16_A           = new(16,             "1021",             "c6c6", true,  true,                   0)
  CRC16_KERMIT      = new(16,             "1021",                  0, true,  true,                   0)
  CRC16_MODBUS      = new(16,             "8005",                 ~0, true,  true,                   0)
  CRC16_X_25        = new(16,             "1021",                  0, true,  true,                  ~0)
  CRC16_XMODEM      = new(16,             "1021",                  0, false, false,                  0)
  CRC24_Radix_64    = new(24,         "00864cfb",                  0, false, false,                  0)
  CRC24_OPENPGP     = new(24,         "00864cfb",         "00b704ce", false, false,                  0)
  CRC24_BLE         = new(24,         "0000065b",         "00555555", true,  true,                   0)
  CRC24_FLEXRAY_A   = new(24,         "005d6dcb",         "00fedcba", false, false,                  0)
  CRC24_FLEXRAY_B   = new(24,         "005d6dcb",         "00abcdef", false, false,                  0)
  CRC24_INTERLAKEN  = new(24,         "00328b63",                  0, false, false,                 ~0)
  CRC24_LTE_A       = new(24,         "00864cfb",                  0, false, false,                  0)
  CRC24_LTE_B       = new(24,         "00800063",                  0, false, false,                  0)
  CRC30_CDMA        = new(30,         "2030b9c7",                  0, false, false,                 ~0)
  CRC31_PHILIPS     = new(31,         "04c11db7",                  0, false, false,                 ~0)
  CRC32             = new(32,         "04c11db7",                  0, true,  true,                  ~0)
  CRC32_BZIP2       = new(32,         "04c11db7",                  0, false, false,                 ~0)
  CRC32C            = new(32,         "1edc6f41",                  0, true,  true,                  ~0)
  CRC32D            = new(32,         "a833982b",                  0, true,  true,                  ~0)
  CRC32_MPEG_2      = new(32,         "04c11db7",                 ~0, false, false,                  0)
  CRC32_POSIX       = new(32,         "04c11db7",                 ~0, false, false,                 ~0)
  CRC32Q            = new(32,         "814141ab",                  0, false, false,                  0)
  CRC32_JAMCRC      = new(32,         "04c11db7",                 ~0, true,  true,                   0)
  CRC32_XFER        = new(32,         "000000af",                  0, false, false,                  0)
  CRC40_GSM         = new(40, "0000000004820009",                 ~0, false, false,                 ~0)
  CRC64_XZ          = new(64, "42f0e1eba9ea3693",                  0, true,  true,                  ~0)
  CRC64_JONES       = new(64, "ad93d23594c935a9",                 ~0, true,  true,                   0)
  CRC64_ECMA        = new(64, "42f0e1eba9ea3693",                  0, false, false,                  0)
  CRC64_WE          = new(64, "42f0e1eba9ea3693",                  0, false, false,                 ~0)
  CRC64_ISO         = new(64, "000000000000001b",                  0, true,  true,                   0)
end
