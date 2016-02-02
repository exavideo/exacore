require 'rubygems'
require_relative 'replay'
require 'io/console'

module Replay
    class ReplayMultiviewer
        # throw a Ruby-ish frontend on the ugly C++
        def add_source(opts={})
            params = ReplayMultiviewerSourceParams.new
            params.source = opts[:port] || fail("Need some kind of input")
            params.x = opts[:x] || 0
            params.y = opts[:y] || 0

            real_add_source(params)
        end
    end

    class RolloutApp
        def initialize
            @dpys = FramebufferDisplaySurface.new
            @multiviewer = ReplayMultiviewer.new(@dpys)

            output = Replay::create_decklink_output_adapter_with_audio(2, 0, RawFrame::CbYCrY8422)
            @program = ReplayPlayout.new(output)
            @preview = RolloutPreview.new

            @multiviewer.add_source(:port => @preview.monitor,
                :x => 0, :y => 0)
            @multiviewer.add_source(:port => @program.monitor,
                :x => 960, :y => 0)
        end

        def start
            @preview.load_file("/home/armena/rollout/sports_intro.mov")
            @preview.seek(15000000)
            @multiviewer.start
        end

        def roll_clip
            file, time = @preview.get
            puts "#{file} at time #{time}"
            @program.lavf_playout(file, time)
        end

        def load_file(filename)
            @preview.load_file(filename)
        end

        def seek(delta)
            @preview.seek(delta)
        end
    end
end

app = Replay::RolloutApp.new
app.start

# Thanks to acook on GitHub (https://gist.github.com/acook/4190379)
def read_char
    STDIN.echo = false
    STDIN.raw!

    input = STDIN.getc.chr
    if input == "\e" then
        input << STDIN.read_nonblock(3) rescue nil
        input << STDIN.read_nonblock(2) rescue nil
    end
ensure
    STDIN.echo = true
    STDIN.cooked!

    return input
end

while true
    key = read_char

    case key
        # left arrow
        when "\e[D"
            app.seek(-1000000)
        # right arrow
        when "\e[C"
            app.seek(1000000)
        when "\r"
            app.roll_clip
        when "q"
            break
        when "l"
            puts ""
            puts "Please input file name!"
            filename = gets.chomp
            if File.readable?(filename)
                app.load_file(filename)
            else
                puts "Can't read that file!"
            end
    end
end
