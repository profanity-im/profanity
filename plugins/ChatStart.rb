module ChatStart

    @@contacts = [
        "\"Prof 2\"",
        "prof3@panesar"
    ]

    def self.prof_on_connect()
        @@contacts.each { | contact |
            Prof::send_line("/msg " + contact)
        }
        Prof::send_line("/win 1")
    end
end
