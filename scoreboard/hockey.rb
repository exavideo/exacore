# Copyright 2011 Andrew H. Armenia.
# 
# This file is part of openreplay.
# 
# openreplay is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# openreplay is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with openreplay.  If not, see <http://www.gnu.org/licenses/>.

class HockeyPenalty
    def initialize(player, is_major)
        @clock = nil
        @player = player
        
        if is_major
            @duration = 3000 # = 300 sec = 5 min
        else
            @duration = 1200 # = 120 sec = 2 min
        end

        @is_major = is_major
        @start_time = nil
    end

    def player
        return @player
    end

    def start_now(clock)
        fail unless @start_time == nil
        @clock = clock
        @start_time = clock.value
    end

    def time_left
        if @clock  
            @start_time + @duration - @clock.value
        else
            @duration
        end
    end

    def expired?
        if @clock.value >= @start_time + @duration
            true
        else
            false
        end
    end

    def expiry_time
        if @start_time == nil
            nil
        else
            @start_time + @duration
        end
    end

    def major?
        @is_major
    end

    def start_time
        @start_time
    end
end


class HockeyPenaltyQueue
    def initialize(clock)
        @active = PQ.new
        @deferred = PQ.new

        # used in ambiguous situations... details later
        @schrodinger = PQ.new

        @clock = clock

        @on_penalty_ended = nil
    end

    def strength
        5 - @active.length
    end

    def time_to_strength_change
        if @active.length == 0
            0
        elsif @active.length == 1 
            if @deferred.length != 0
                # a single double-minor
                @active[0].time_left + @deferred[0].time_left
            else
                @active[0].time_left
            end
        elsif @active.length == 2
            # start with the active penalties
            active0 = @active[0].time_left
            active1 = @active[1].time_left
            players0 = [ @active[0].player ]
            players1 = [ @active[1].player ]

            # stack the deferred penalties on top of active ones...
            # if the deferred penalty is on the same player as an active
            # penalty, stack the deferred penalty on top of the same player.
            # Otherwise, stack it on the one that expires first.
            @deferred.each do |p|
                if players0.include? p.player 
                    active0 += p.time_left
                elsif players1.include? p.player
                    active1 += p.time_left
                elsif active0 < active1 
                    active0 += p.time_left
                    players0 << p.player
                else
                    active1 += p.time_left
                    players1 << p.player
                end
            end

            if active0 < active1
                active0
            else
                active1
            end
        end
    end

    def on_penalty_ended(&block)
        if block.arity > 0 and block.arity != 1
            fail 'block must take one argument'
        end
        @on_penalty_ended = block
    end

    def add_penalty(p)
        # check if there are already active penalties for this player
        if (@active.has_player?(p.player))
            swap = @schrodinger.find { |pair| pair[1].player == p.player }
            if swap.nil?
                @deferred << p
            else
                idx = @active.find_index(swap[1])
                @schrodinger.delete(swap)
                @active[idx] = swap[0]
            end
        else
            @deferred << p
        end
    end

    def update
        new_active = PQ.new
        new_deferred = PQ.new

        expiry_times = []

        @active.each do |penalty|
            if penalty.expired?
                expiry_times << penalty.expiry_time
                finish_penalty(penalty)
            else
                new_active << penalty
            end
        end

        @deferred.each do |penalty|
            # undefer any penalties we can... i.e. if the player is not 
            # already penalized for something else and not more than two
            # are already active. Otherwise, just leave them deferred.
            if (new_active.length < 2 and not 
                    new_active.has_player?(penalty.player))
                penalty.start_now(@clock)
                new_active << penalty
            else
                new_deferred << penalty
            end
        end

        @active = new_active
        @deferred = new_deferred
    end

    def drop_penalty
        # check active penalties for minors
        all_minors = @active.select { |p| not p.major? }
        
        # case 1: one minor
        if all_minors.length == 1
            # drop it
            @active.delete(all_minors[0])
            finish_penalty(all_minors[0])
        # case 2: two minors
        elsif all_minors.length == 2
            # if start times not equal
            if all_minors[0].start_time < all_minors[1].start_time
                # delete first
                @active.delete(all_minors[0])
                finish_penalty(all_minors[0])
            # if equal
            elsif all_minors[0].start_time == all_minors[1].start_time
                # if exactly one is double minor, end the one that isn't
                if (@deferred.has_player?(all_minors[0].player) and not @deferred.has_player(all_minors[1].player))
                    @active.delete(all_minors[1])
                    finish_penalty(all_minors[1])
                elsif (@deferred.has_player?(all_minors[1].player) and not @deferred.has_player(all_minors[0].player))
                    @active.delete(all_minors[0])
                    finish_penalty(all_minors[0])
                else
                    @active.delete(all_minors[0])
                    finish_penalty(all_minors[0])
                    # Add both penalties to Schrodinger's penalty box.
                    # Thus, we can fix our wrong guess if a player who we 
                    # think is already penalized gets another.
                    @schrodinger << [ all_minors[0], all_minors[1] ]
                end
            end
        end
                
    end

protected
    def finish_penalty(p)
        if @on_penalty_ended
            @on_penalty_ended.call(p)
        end
    end

    class PQ < Array
        def has_player?(player)
            # ruby is win
            any? { |penalty| penalty.player == player }
        end
    end
end

class HockeyGame
    def initialize(clock)
        # score
        @home_score = 0
        @away_score = 0

        # clock
        @clock = clock
        @period = 1

        # penalty queues
        @home_penalties = []
        @home_deferred_penalties = []
        @away_penalties = []
        @away_deferred_penalties = []

        # who called any timeout
        @home_timeout = false
        @away_timeout = false

        # where to call magic functions when things happen
        @event_receiver = nil
    end

    def event_receiver=(evtrcv)
        @event_receiver = evtrcv
    end

    # Timeouts
    def call_home_timeout
        if not @home_timeout and not @away_timeout
            @home_timeout = true
            fire_event('home_timeout_begin')
            fire_event('timeout_begin', :home)
        end
    end

    def call_away_timeout
        if not @home_timeout and not @away_timeout
            @away_timeout = true
            fire_event('away_timeout_begin')
            fire_event('timeout_begin', :away)
        end
    end

    def end_timeouts
        if @home_timeout
            @home_timeout = false
            fire_event('home_timeout_end')
            fire_event('timeout_end', :home)
        elsif @away_timeout
            @away_timeout = false
            fire_event('away_timeout_end')
            fire_event('timeout_end', :away)
        end
    end

    # Goal scoring
    def score_home_goal
        @home_score += 1
        fire_event('home_goal')
        fire_event('goal', :home)
    end

    def score_away_goal
        @away_score += 1
        fire_event('away_goal')
        fire_event('goal', :away)
    end

    # Announcement of goal information
    def announce_home_goal_info(scored, *assists)
        fire_event('home_goal_info_ready', scored, *assists)
        fire_event('goal_info_ready', :home, scored, *assists)
    end

    def announce_away_goal_info(scored, *assists)
        fire_event('away_goal_info_ready', scored, *assists)
        fire_event('goal_info_ready', :away, scored, *assists)
    end

    # Delayed penalty
    def call_delayed_penalty
        fire_event('unknown_delayed_penalty')
        fire_event('delayed_penalty', :unknown)
    end
    
    def call_home_delayed_penalty
        fire_event('home_delayed_penalty')
        fire_event('delayed_penalty', :home)
    end

    def call_away_delayed_penalty
        fire_event('away_delayed_penalty')
        fire_event('delayed_penalty', :away)
    end

    # Penalty data input
    def call_home_penalty(player, penalty, is_major)
        fire_event('home_penalty_data_available', player, penalty, is_major)
        fire_event('penalty_data_available', :home, player, penalty, is_major)

        # FIXME add to penalty queue
        penalty = HockeyPenalty.new(player, is_major)
    end

    def call_away_penalty(player, penalty, is_major)
        fire_event('away_penalty_data_available', player, penalty, is_major)
        fire_event('penalty_data_available', :away, player, penalty, is_major)

        # FIXME add to penalty queue
    end

    def update
        # squirrel away old strength data
        old_home_strength = 5 - @home_penalties.length
        old_away_strength = 5 - @away_penalties.length

        # update the queues... remove expired penalties,
        # add newly-active ones
        @home_penalties, @home_deferred_penalties = 
            update_pq(@home_penalties, @home_deferred_penalties)

        @away_penalties, @away_deferred_penalties =
            update_pq(@away_penalties, @away_deferred_penalties)

        # compare current strength to previous strength... 
        # fire appropriate events
        new_home_strength = 5 - @home_penalties.length
        new_away_strength = 5 - @away_penalties.length

        unless (new_home_strength == old_home_strength and
                new_away_strength == old_away_strength)
            fire_event("strength_changed", 
                    old_home_strength, old_away_strength, 
                    new_home_strength, new_away_strength
            )
        end
    end

protected
    # Find removal candidate from a team's queue if they are scored upon.
    def penalty_to_remove(active, deferred)
        # If one penalty...
            # If it's minor...
                # That one ends
            # Otherwise
                # None ends
        # If two penalties
            # If both are major
                # None ends
            # If one is major
                # Minor ends
            # If both are minor
                # If both have different start times
                    # First one queued ends
                # If both have same start time
                    # If any double minors
                        # End one w/o double minor
                    # Otherwise
                        # End one at random. See below for dealing with this.
        
        # IMPORTANT NOTE: This creates a situation whereby there is 
        # uncertainty. When we end a random penalty - i.e. without knowinf
        # for sure who came out, we send that player to Schrodinger's penalty
        # box. When penalties are called, we check the penalty queues for that
        # player. If a penalty is called on a player who was still in the 
        # penalty queue... the wavefunction collapses! We know the penalty
        # was really still on the guy in Schrodinger's penalty box. So...
        # we remove the bogus penalty, move the one guy from Schrodinger's
        # box to the real active penalty queue, and create a new penalty to
        # replace our mistaken one.
    end
    
    def fire_event(event, *args)
        if @event_receiver.respond_to? event
            m = @event_receiver.method(event)
            if m.arity == args.length or m.arity == -1
                m.call(*args)
            end
        end
    end
end
