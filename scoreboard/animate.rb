module Animate
    class Manager
        def initialize
            @active_animations = []
        end

        def update
            @active_animations.each { |ani| ani.update }
        end

        def start_animation(animation)
            animation.on_done do
                @active_animations.delete animation
            end

            @active_animations << animation
        end
    end

    class Animation
        def initialize
            @done_handlers = []
        end

        def on_done(&block)
            @done_handlers << block
        end

        protected
        def call_done_handlers
            @done_handlers.each do |handler|
                handler.call
            end
        end
    end

    class Blink < Animation
        attr_accessor :default, :blink, :blink_period

        def initialize
            super
            @default = "#000000"
            @blink = "#ffffff"
            @active = false
            @frames = 0
            @blink_period = 10
            @n_blinks = 6
        end

        def action(&block)
            @action = block
        end

        def update
            @frames += 1
            if @frames >= @n_blinks * @blink_period
                @action.call(@default)
                call_done_handlers
            else
                if (@frames / @blink_period) % 2 == 0
                    @action.call(@blink)
                else
                    @action.call(@default)
                end
            end
        end
    end

    class Linear < Animation
        def initialize(start, finish, frames)
            super()
            @start = start
            @finish = finish
            @frames = 0
            @total_frames = frames
        end

        def action(&block)
            @action = block
        end

        def update
            @frames += 1
            if @frames >= @total_frames
                @action.call(@finish)
                call_done_handlers
            else
                @action.call(@start + (@finish - @start) * 
                    @frames.to_f / @total_frames.to_f)
            end
        end
    end

    class HalfCosine < Animation
        def initialize(start, finish, frames)
            super()
            @start = start
            @finish = finish
            @frames = 0
            @total_frames = frames
        end

        def action(&block)
            @action = block
        end

        def update
            @frames += 1
            if @frames >= @total_frames
                @action.call(@finish)
                call_done_handlers
            else
                linear = @frames.to_f / @total_frames.to_f
                fraction = 0.5 - 0.5 * Math.cos(linear * 3.14159)

                @action.call(@start + (@finish - @start) * fraction)
            end
        end
    end
end

