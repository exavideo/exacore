#!/usr/bin/env ruby

require 'erb'

class HockeyState
    def initialize
        @tenths = 12000 # twenty minutes
        @tenths = 473
        @period = 3

        # we need a good looking score for testing :)
        @home_team = "RPI"
        @home_score = 1
        @away_team = "UNION"
        @away_score = 0

        @home_bgcolor = "#ff0000"
        @home_fgcolor = "#ffffff"
        @away_bgcolor = "#800000"
        @away_fgcolor = "#ffffff"

        @delayed_penalty = false
        @flag_team = nil
        @arb_text = nil
    end

    attr_reader :home_bgcolor, :home_fgcolor
    attr_reader :away_bgcolor, :away_fgcolor
    attr_reader :home_team, :home_score, :away_team, :away_score 
    attr_reader :flag_team
    attr_reader :period
    attr_reader :delayed_penalty, :full_strength, :arb_text

    def clock
        if @tenths > 600
            sprintf "%d:%02d", @tenths / 600, (@tenths % 600) / 10
        else
            sprintf ":%02d.%d", @tenths / 10, @tenths % 10
        end
    end

    def advance_clock
        @tenths -= 1
    end

    def do_template(svgerb)
        svgerb.result(binding)
    end
end

state = HockeyState.new
template = nil

File.open('test_files/scoreboard.svg.erb', 'r') do |f|
    template = ERB.new(f.read( ))
end

i = 0

while true
    # await request
    dummy = STDIN.read(1)
    if dummy.nil?
        break
    end

    # render to SVG
    svg = state.do_template(template)

    size = [ svg.length ].pack('L')

    STDOUT.write(size)
    STDOUT.write(svg)

    i += 1

    if i == 3
        i = 0
        state.advance_clock
    end
end
