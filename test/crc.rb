
assert("CRC is class") do
  assert_equal Class, CRC.class
end

assert("CRC is meta class") do
  assert_raise(TypeError) { CRC.bitsize }
  assert_raise(TypeError) { CRC.polynomial }
  assert_raise(TypeError) { CRC.initial_crc }
  assert_raise(TypeError) { CRC.reflect_input? }
  assert_raise(TypeError) { CRC.reflect_output? }
  assert_raise(TypeError) { CRC.xor_output }
end

assert("CRC::CRC32") do
  assert_equal 32, CRC::CRC32.bitsize
  if (1 << 28).kind_of?(Float)
    assert_equal "04c11db7", CRC::CRC32.polynomial
  else
    assert_equal 0x04c11db7, CRC::CRC32.polynomial
  end
  assert_equal 0, CRC::CRC32.initial_crc
  assert_true CRC::CRC32.reflect_input?
  assert_true CRC::CRC32.reflect_output?
  assert_equal (-1), CRC::CRC32.xor_output
end

assert("CRC::CRC32.hexdigest") do
  assert_equal "00000000", CRC::CRC32.hexdigest(nil)
  assert_equal "00000000", CRC::CRC32.hexdigest("")
  assert_equal "cbf43926", CRC::CRC32.hexdigest("123456789")
  assert_equal "3e29169c", CRC::CRC32.hexdigest("123456789" * 4)
end

assert("CRC::CRC32 streaming") do
  crc32 = CRC::CRC32.new
  assert_kind_of CRC::CRC32, crc32
  assert_equal "00000000", crc32.hexdigest
  assert_kind_of CRC::CRC32, crc32.update("")
  assert_equal "00000000", crc32.hexdigest
  assert_kind_of CRC::CRC32, crc32.update("123456789")
  assert_equal "cbf43926", crc32.hexdigest
  assert_kind_of CRC::CRC32, crc32.update("123456789" * 3)
  assert_equal "3e29169c", crc32.hexdigest
end
