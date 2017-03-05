MRuby::Gem::Specification.new("crc") do |s|
  s.summary = "general CRC calcurator"
  s.version = "0.2"
  s.license = "BSD-2-Clause"
  s.author  = "dearblue"
  s.homepage = "https://github.com/dearblue/mruby-crc"

  if s.cc.command =~ /\b(?:g?cc|clang)\d*\b/
    s.cc.flags << "-Wall" <<
                  "-Wno-shift-negative-value" <<
                  "-Wno-shift-count-negative" <<
                  "-Wno-shift-count-overflow" <<
                  "-Wno-missing-braces"
  end
end
