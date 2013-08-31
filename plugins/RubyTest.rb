module RubyTest

    def self.prof_init(version, status)
        Prof::cons_show("RubyTest: init, " + version + ", " + status)
        Prof::register_command("/ruby", 0, 1, "/ruby", "RubyTest", "RubyTest", cmd_ruby)
        Prof::register_timed(timer_test, 10)
    end

    def self.prof_on_start()
        Prof::cons_show("RubyTest: on_start")
    end

    def self.prof_on_connect(account_name, fulljid)
        Prof::cons_show("RubyTest: on_connect, " + account_name + ", " + fulljid)
    end

    def self.prof_on_message_received(jid, message)
        Prof::cons_show("RubyTest: on_message_received, " + jid + ", " + message)
        Prof::cons_alert
        return message + "[RUBY]"
    end

    def self.prof_on_message_send(jid, message)
        Prof::cons_show("RubyTest: on_message_send, " + jid + ", " + message)
        Prof::cons_alert
        return message + "[RUBY]"
    end

    def self.cmd_ruby()
        return Proc.new { | msg |
            if msg
                Prof::cons_show("RubyTest: /ruby command called, arg = " + msg)
            else
                Prof::cons_show("RubyTest: /ruby command called with no arg")
            end
            Prof::cons_alert
            Prof::notify("RubyTest: notify", 2000, "Plugins")
            Prof::send_line("/help")
            Prof::cons_show("RubyTest: sent \"/help\" command")
        }
    end

    def self.timer_test()
        return Proc.new {
            Prof::cons_show("RubyTest: timer fired.")
            recipient = Prof::get_current_recipient
            if recipient
                Prof::cons_show("  current recipient = " + recipient)
            end
            Prof::cons_alert
        }
    end
end
