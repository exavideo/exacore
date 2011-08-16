require_relative '../input/shuttlepro'

class TestShuttleProInput < ShuttleProInput
    def on_shuttle(value)
        puts "shuttle: #{value}"
    end

    def on_jog(delta)
        puts "jog: #{delta}"
    end

    def on_button_down(button)
        puts "button down: #{button}"
    end

    def on_button_up(button)
        puts "button up: #{button}"
    end
end

obj = TestShuttleProInput.new

while true
    Thread.pass
end
