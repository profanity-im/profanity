module RubyTest
    include prof

    prof_version = ""
    prof_status = ""

    def prof_init(version, status)
        prof_version = version
        prof_status = status
    end

    def prof_on_start()
        prof.cons_show("RubyTest: " + prof_version + " " + prof_status)
    end
end
