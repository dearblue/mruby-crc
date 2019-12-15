#ruby

%w(32 16 64).each do |intsize|
  name = %(host#{intsize == "32" ? "" : intsize})

  MRuby::Build.new(name) do |conf|
    toolchain :gcc

    conf.build_dir = name

    enable_debug
    enable_test

    cc.command = "gcc-7"
    cxx.command = "g++-7"
    cc.defines = %W(MRB_INT#{intsize})

    gem core: "mruby-print"
    gem core: "mruby-bin-mrbc"
    gem __dir__ do
      cc.flags << %w(-Wall -std=c11)
    end
  end
end
