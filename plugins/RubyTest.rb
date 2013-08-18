module RubyTest
    def RubyTest.prof_init(version, status)
        Prof::cons_show("RubyTest: init, " + version + ", " + status)
    end

    def RubyTest.prof_on_start()
        Prof::cons_show("RubyTest: on_start")
    end

    def RubyTest.prof_on_connect()
        Prof::cons_show("RubyTest: on_connect")
    end

    def RubyTest.prof_on_message_received(jid, message)
        Prof::cons_show("RubyTest: on_message_received, " + jid + ", " + message)
    end
end
