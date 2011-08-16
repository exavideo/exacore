require_relative 'input'

class ShuttleProInput
    def initialize
        @last_jog = nil
        @device = File.open("/dev/shuttlepro")

        Thread.new { 
            begin
                poll_events 
            rescue => e
                puts e
                print e.backtrace.join("\n")
            end
        }
    end
    
    def on_shuttle(value)
        
    end

    def on_jog(delta)

    end

    def on_button_down(button)

    end

    def on_button_up(button)

    end

private
    
    def poll_events
        # ugh... C++ in my Ruby...
        evt = Input::InputEvent.new

        while true
            if Input::read_event(@device.fileno, evt) != 0
                puts "event received"
                if evt.type == 2 and evt.code == 7
                    jog = evt.value
                    if @last_jog
                        # this is kludgy because it has to account for wrap.
                        if jog == 0 and @last_jog == 255
                            on_jog(1)
                        elsif jog == 255 and @last_jog == 0
                            on_jog(-1)
                        elsif jog - @last_jog == 1
                            on_jog(1)
                        elsif jog - @last_jog == -1
                            on_jog(-1)
                        else
                            puts "warning: jog dial misbehaving?"
                        end
                    end
                    @last_jog = jog
                elsif evt.type == 2 and evt.code == 8
                    # there's no "zero" position on the shuttle dial
                    # we'll make -1 and +1 act as zero instead
                    if evt.value > 0
                        on_shuttle(evt.value - 1)                     
                    else
                        on_shuttle(evt.value + 1)
                    end
                elsif evt.type == 1
                    if evt.value == 0
                        on_button_up(evt.code)
                    else
                        on_button_down(evt.code)
                    end
                else
                    puts "unknown event??"
                end
            else
                puts "read error from shuttle controller!"
                break
            end
        end
    end
end
