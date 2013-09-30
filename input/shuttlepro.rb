require_relative 'input'

class ShuttleProInput
    def initialize
        @last_jog = nil
        #@device = File.open("/dev/shuttlepro")

        # search for ShuttlePRO devices
        devices = Dir.glob('/dev/input/by-id/usb-Contour*')
        fail "No ShuttlePRO found" if devices.length == 0
        @device = File.open(devices[0])

        Thread.new { 
            loop do
                begin
                    poll_events 
                rescue => e
                    puts e
                    print e.backtrace.join("\n")
                end
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
            # wait until data is readable (allowing other threads to run)
            IO.select([@device])
            if Input::read_event(@device.fileno, evt) != 0
                if evt.type == 2 and evt.code == 7
                    jog = evt.value
                    if @last_jog
                        # jog > last_jog means that we are going clockwise.
                        # last_jog > jog means that we are going counter-clockwise.
                        # The problem is that all of this is modulo 256. So, applying
                        # Occam's razor, we figure out what the rotation would be
                        # in both directions, and take whichever works out to a 
                        # smaller rotation. :)
                        cw_rotation = (jog - @last_jog) % 256
                        ccw_rotation = (@last_jog - jog) % 256
                        
                        if (cw_rotation < ccw_rotation)
                            on_jog(cw_rotation)
                        else
                            on_jog(-ccw_rotation)
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
                end
            else
                puts "read error from shuttle controller!"
                break
            end
        end
    end
end
