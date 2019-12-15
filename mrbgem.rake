MRuby::Gem::Specification.new("mruby-crc") do |s|
  s.summary = "general CRC calcurator"
  s.version = File.read(File.join(File.dirname(__FILE__), "README.md")).scan(/^\s*[\-\*] version:\s*(\d+(?:\.\d+)+)/i).flatten[-1]
  s.license = "BSD-2-Clause"
  s.author  = "dearblue"
  s.homepage = "https://github.com/dearblue/mruby-crc"

  add_dependency "mruby-aux", github: "dearblue/mruby-aux"

  if s.cc.command =~ /\b(?:g?cc|clang)\d*\b/
    s.cc.flags << "-Wno-shift-negative-value" <<
                  "-Wno-shift-count-negative" <<
                  "-Wno-shift-count-overflow" <<
                  "-Wno-missing-braces"
  end
end
