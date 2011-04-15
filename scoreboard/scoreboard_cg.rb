#!/usr/bin/env ruby 

require 'socket'
require 'erb'
require './scoreboard/animate'

# Tracks the game clock (in tenths of a second)
class GameClock
    def initialize(start_tenths)
        @started_tenths = start_tenths.to_i
        @running = false
    end

    def start
        if @started_tenths > 0
            @started_time = Time.now
            @running = true
        end
    end

    def stop
        @started_tenths = time
        @running = false
    end

    def sync(new_tenths)
        delta_tenths = new_tenths - time
        @started_tenths += delta_tenths.to_i
    end

    def time
        if @running
            delta_t = Time.now - @started_time
            STDERR.puts "delta_t = #{delta_t}"
            delta_tenths = ((delta_t.to_f) * 10).to_i
            STDERR.puts "delta_tenths = #{delta_tenths}"
            if @started_tenths - delta_tenths > 0
                @started_tenths - delta_tenths
            else
                0
            end
        else
            @started_tenths
        end
    end

    def to_s
        t = time
        if t >= 600
            sprintf "%d:%02d", t / 600, (t / 10) % 60
        else
            sprintf "%02d.%d", t / 10, t % 10
        end
    end
end

class BaseState
    def id
        "Base 0.1"
    end

    def commands
        { }
    end

    def do_command(cmd)
        args = cmd.chomp.split ' '
        cmd = args.shift

        if commands.has_key? cmd
            STDERR.puts "executing: #{cmd}"
            commands[cmd].call(*args)
        else
            STDERR.puts "invalid command #{cmd}"
            false
        end
    end
end



class HockeyState < BaseState
    def initialize
        # start with 20 minutes on the clock in the 1st period
        #@game_clock = GameClock.new(20 * 60 * 10)
        @game_clock = GameClock.new(599)
        @period = 1

        @home_dropdown_offset = 0
        @away_dropdown_offset = 0
        @common_dropdown_offset = 0

        # we need a good looking score for testing :)
        @home_team = "RPI"
        @home_score = 1
        @home_fill = "#Home_Team_Score_Box_1_"
        @away_team = "COL"
        @away_score = 0
        @away_fill = "#Home_Team_Score_Box_4"

        @home_text = nil
        @away_text = nil
        @common_text = nil

        @home_power_play = false
        @away_power_play = false
        
        @anim_mgr = Animate::Manager.new

        @home_bgcolor = "#ff0000"
        @home_fgcolor = "#ffffff"
        @away_bgcolor = "#800000"
        @away_fgcolor = "#ffffff"

        @home_score_background_color = "#666666"
        @away_score_background_color = "#666666"
        @home_score_text_color = "#ffffff"
        @away_score_text_color = "#ffffff"

        @flag_team = nil

        @clock_opacity = 1.0
        @clock_xofs = 0
        @clock_yofs = 0
        @scores_opacity = 1.0
        @scores_xofs = 0
        @scores_yofs = 0
        @bug_opacity = 1.0

        @text_up = false
        @text_opacity = 0
        @text_bgcolor = "#dfd800"
        @text_fgcolor = "#000000"
        @text_line = ''
        @global_xofs = 0
        @global_yofs = 0

        @home_redscore_opacity = 0.0
        @away_redscore_opacity = 0.0

        # Commands we can accept
        @commands = {
            'start_clock' => Proc.new { 
                @game_clock.start
                true
            },

            'stop_clock' => Proc.new {
                @game_clock.stop
                true
            },

            'home_goal' => Proc.new {
                @home_score += 1
                home_goal_blink
                true
            },

            'away_goal' => Proc.new {
                @away_score += 1
                away_goal_blink
                true
            },

            'home_down' => Proc.new {
                home_dropdown_in             
            },

            'home_up' => Proc.new {
                home_dropdown_out
            },

            'home_text' => Proc.new { |*args|
                @home_text = args.join(' ')
            },

            'dissolve_in' => Proc.new {
                start_dissolve_in
            },

            'dissolve_out' => Proc.new {
                start_dissolve_out
            }
        }
    end

    def commands
        @commands
    end

    def home_goal_blink
        sine_blink = Animate::Sine.new(0.0, 1.0, 90, 6)
        sine_blink.action do |value|
            @home_redscore_opacity = value
        end
        @anim_mgr.start_animation(sine_blink)
    end

    def away_goal_blink
        sine_blink = Animate::Sine.new(0.0, 1.0, 90, 6)
        sine_blink.action do |value|
            @away_redscore_opacity = value
        end
        @anim_mgr.start_animation(sine_blink)
    end
    
    def home_dropdown_in
        slide_down = Animate::HalfCosine.new(-37, 0, 15)
        slide_down.action do |value|
            @home_dropdown_offset = value
        end
        @anim_mgr.start_animation(slide_down)
    end

    def home_dropdown_out
        slide_up = Animate::HalfCosine.new(0, -37, 30)
        slide_up.action do |value|
            @home_dropdown_offset = value
        end
        @anim_mgr.start_animation(slide_up)
    end


    def start_dissolve_out
        scores_slide = Animate::HalfCosine.new(0, 100, 30)
        scores_slide.action do |value|
            @scores_xofs = value
        end

        scores_dissolve = Animate::HalfCosine.new(1, 0, 30)
        scores_dissolve.action do |value|
            @scores_opacity = value
        end

        clock_slide_x = Animate::HalfCosine.new(0, -30.1, 30)
        clock_slide_x.action do |value|
            @clock_xofs = value
        end

        clock_slide_y = Animate::HalfCosine.new(0, 95.3, 30)
        clock_slide_y.action do |value|
            @clock_yofs = value
        end

        clock_dissolve = Animate::HalfCosine.new(1, 0, 30)
        clock_dissolve.action do |value|
            @clock_opacity = value
        end

        clock_dissolve.on_done do
            finish_dissolve_out
        end

        @anim_mgr.start_animation(scores_slide)
        @anim_mgr.start_animation(scores_dissolve)
        @anim_mgr.start_animation(clock_dissolve)
        #@anim_mgr.start_animation(clock_slide_x)
        @anim_mgr.start_animation(clock_slide_y)

    end

    def finish_dissolve_out
        bug_dissolve = Animate::HalfCosine.new(1, 0, 30)
        bug_dissolve.action do |value|
            @bug_opacity = value
        end

        @anim_mgr.start_animation(bug_dissolve)
    end

    def start_dissolve_in
        bug_dissolve = Animate::HalfCosine.new(0, 1, 30)
        bug_dissolve.action do |value|
            @bug_opacity = value
        end

        bug_dissolve.on_done do
            finish_dissolve_in
        end

        @anim_mgr.start_animation(bug_dissolve)
    end

    def finish_dissolve_in
        scores_slide = Animate::HalfCosine.new(100, 0, 30)
        scores_slide.action do |value|
            @scores_xofs = value
        end

        scores_dissolve = Animate::HalfCosine.new(0, 1, 30)
        scores_dissolve.action do |value|
            @scores_opacity = value
        end

        clock_dissolve = Animate::HalfCosine.new(0, 1, 30)
        clock_dissolve.action do |value|
            @clock_opacity = value
        end

        clock_slide_x = Animate::HalfCosine.new(-30.1, 0, 30)
        clock_slide_x.action do |value|
            @clock_xofs = value
        end

        clock_slide_y = Animate::HalfCosine.new(95.3, 0, 30)
        clock_slide_y.action do |value|
            @clock_yofs = value
        end


        @anim_mgr.start_animation(scores_slide)
        @anim_mgr.start_animation(scores_dissolve)
        @anim_mgr.start_animation(clock_dissolve)
        #@anim_mgr.start_animation(clock_slide_x)
        @anim_mgr.start_animation(clock_slide_y)
    end

    attr_reader :home_bgcolor, :home_fgcolor
    attr_reader :away_bgcolor, :away_fgcolor
    attr_reader :home_team, :home_score, :away_team, :away_score 
    attr_reader :flag_team
    attr_reader :period
    attr_reader :home_score_text_color, :away_score_text_color
    attr_reader :home_score_background_color, :away_score_background_color

    attr_reader :clock_opacity, :clock_xofs, :clock_yofs
    attr_reader :scores_opacity, :scores_xofs, :scores_yofs
    attr_reader :bug_opacity

    attr_reader :global_xofs, :global_yofs
    attr_reader :text_opacity, :text_bgcolor, :text_fgcolor, :text_line

    attr_reader :home_text, :away_text, :common_text
    attr_reader :home_power_play, :away_power_play
    attr_reader :home_dropdown_offset, :away_dropdown_offset, :common_dropdown_offset

    attr_reader :home_fill, :away_fill

    attr_reader :home_redscore_opacity, :away_redscore_opacity

    def id
        "Hockey 0.1"
    end

    def clock
        @game_clock.to_s
    end

    def do_template(svgerb)
        ret = svgerb.result(binding)
        @anim_mgr.update
        ret
    end

    def home_power_play_time
        '2:00'
    end

    def away_power_play_time
        '2:00'
    end
end

state = HockeyState.new

template = File.open('test_files/kemper_scoreboard.svg.erb', 'r') do |f|
    ERB.new(f.read( ))
end

Thread.new do
    ls = TCPServer.new('0.0.0.0', 30005)
    loop do
        Thread.new(ls.accept) do |client|
            STDERR.print "accepted connection from ", client.peeraddr[2], "\n"

            client.write state.id
            client.write "\n"

            loop do
                begin
                    cmd = client.readline
                    STDERR.puts cmd
                    
                    result = begin
                        Thread.exclusive do
                            state.do_command(cmd)
                        end
                    rescue => e
                        STDERR.puts e.inspect
                        STDERR.puts e.backtrace.join("\n")
                    end
                        
                    #result = true

                    if result
                        client.write "OK\n"
                    else
                        client.write "FAIL\n"
                    end

                rescue EOFError
                    client.close
                    break
                end
            end
            STDERR.print "client ", client.peeraddr[2], " disconnected"
        end
    end
end

while true
    # await request
    dummy = STDIN.read(1)
    if dummy.nil?
        break
    end

    # render to SVG (making sure state doesn't change while that happens)
    svg = Thread.exclusive do
        state.do_template(template)
    end

    size = [ svg.length ].pack('L')

    STDOUT.write(size)
    STDOUT.write(svg)
end
