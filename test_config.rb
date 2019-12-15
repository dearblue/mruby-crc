%w(32 16 64).each do |intsize|
  name = %(host#{intsize == "32" ? "" : intsize})

  MRuby::Build.new(name) do |conf|
    toolchain :clang

    conf.build_dir = name

    enable_debug
    enable_test

    cc.defines = %W(MRB_INT#{intsize})

    gem core: "mruby-print"
    gem core: "mruby-bin-mirb"
    gem core: "mruby-bin-mrbc"
    gem core: "mruby-bin-mruby"
    gem __dir__ do
      cc.flags << %w(-std=c11 -Wall -pedantic
                     -Wno-gnu-empty-initializer
                     -Wno-zero-length-array
                     -Wno-gnu-zero-variadic-macro-arguments)
    end
  end
end
