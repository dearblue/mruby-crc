MRuby::Build.new("host") do |conf|
  toolchain :clang

  conf.build_dir = "host"

  conf.gem core: "mruby-print"
  conf.gem core: "mruby-bin-mirb"
  conf.gem core: "mruby-bin-mruby"
  conf.gem "."

  conf.cc.flags << "-O0"

  conf.enable_test
end

MRuby::Build.new("host64") do |conf|
  toolchain :clang

  conf.cc.defines = %w(MRB_INT64)

  conf.build_dir = "host64"

  conf.gem core: "mruby-print"
  conf.gem core: "mruby-bin-mrbc"
  conf.gem core: "mruby-bin-mruby"
  conf.gem "."

  conf.cc.flags << "-O0"

  conf.enable_test
end

MRuby::Build.new("host16") do |conf|
  toolchain :clang

  conf.cc.defines = %w(MRB_INT16)

  conf.build_dir = "host16"

  conf.gem core: "mruby-print"
  conf.gem core: "mruby-bin-mrbc"
  conf.gem core: "mruby-bin-mruby"
  conf.gem "."

  conf.cc.flags << "-O0"

  conf.enable_test
end
