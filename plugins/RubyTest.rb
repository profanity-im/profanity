module RubyTest
    def self.prof_init(version, status)
        Prof::cons_show("RubyTest: init, " + version + ", " + status)
        Prof::register_command("/ruby", 0, 1, "/ruby", "RubyTest", "RubyTest", cmd_ruby)
        Prof::register_timed(timer_test, 5)
    end

    def self.prof_on_start()
        Prof::cons_show("RubyTest: on_start")
    end

    def self.prof_on_connect()
        Prof::cons_show("RubyTest: on_connect")
    end

    def self.prof_on_message_received(jid, message)
        Prof::cons_show("RubyTest: on_message_received, " + jid + ", " + message)
    end

    def self.cmd_ruby()
        return Proc.new {
            | msg |

            if msg
                Prof::cons_show("Ruby command called: " + msg)
            else
                Prof::cons_show("Ruby command called with no arg")
            end
        }
    end

    def self.timer_test()
        return Proc.new {
            Prof::cons_show("Ruby timer fired.")
        }
    end
end
