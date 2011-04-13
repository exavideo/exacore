require './hockey'
require 'test/unit'

class TestClock
    def initialize
        @value = 0
    end

    def value
        @value
    end

    def advance(tenths, &block)
        new_value = @value + tenths
        while @value < new_value
            @value += 1
            if block != nil
                block.call
            end
        end
    end

end

class HockeyPenaltyQueueTest < Test::Unit::TestCase
    # common test case setup
    def setup
        @tc = TestClock.new
        @pq = HockeyPenaltyQueue.new(@tc)
    end


    # Player X1 gets a minor penalty. Team X kills the penalty.
    # Check that the timers and strengths operate properly.
    def test_single_minor_killed
        advance_clock(1200)

        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)

        p = HockeyPenalty.new('kennedy', false)
        @pq.add_penalty(p) 
        @pq.update
        
        assert_equal(4, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)
        
        advance_clock 600
        assert_equal(600, @pq.time_to_strength_change)

        advance_clock 300
        assert_equal(300, @pq.time_to_strength_change)

        advance_clock 300 # 3, 2, 1... kill!
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
        
    end

    # Player X1 gets a minor penalty. Team Y scores.
    # Check that the penalty is properly cancelled, and that
    # the timers and strength fields update properly.
    def test_single_minor_not_killed
        advance_clock 1200

        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)

        p = HockeyPenalty.new('zajac', false)
        @pq.add_penalty(p) 
        @pq.update
        
        assert_equal(4, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)
        
        advance_clock 600
        assert_equal(600, @pq.time_to_strength_change)

        advance_clock 300
        assert_equal(300, @pq.time_to_strength_change)

        @pq.drop_penalty # goal!
        @pq.update
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
    end

    # Player X1 gets a major penalty. Team X kills the penalty.
    # Check that timers and strength operate correctly.
    def test_single_major_killed
        # Test the proper progress of a single minor penalty.
        # The penalty is killed. No goal is scored.
        advance_clock 1200

        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)

        p = HockeyPenalty.new('kennedy', true)
        @pq.add_penalty(p) 
        @pq.update
        
        assert_equal(4, @pq.strength)
        assert_equal(3000, @pq.time_to_strength_change)
        
        advance_clock 1500
        assert_equal(1500, @pq.time_to_strength_change)

        advance_clock 500
        assert_equal(1000, @pq.time_to_strength_change)

        advance_clock 1000 
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
        
    end

    # Player X1 gets a major penalty. Team Y scores during the
    # shorthanded situation. Check that X1's penalty is not
    # cancelled, and that the timers advance properly.
    # Also check the end-of-penalty notification callback.
    def test_single_major_not_killed
        advance_clock 1200
        penalties_ended = 0

        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)

        p = HockeyPenalty.new('zajac', true)
        @pq.add_penalty(p) 
        @pq.update

        @pq.on_penalty_ended do |pdone|
            assert_equal(p, pdone)
            penalties_ended += 1
        end
        
        assert_equal(4, @pq.strength)
        assert_equal(3000, @pq.time_to_strength_change)
        assert_equal(0, penalties_ended)
        
        advance_clock 2000
        assert_equal(1000, @pq.time_to_strength_change)
        assert_equal(0, penalties_ended)

        advance_clock 500
        @pq.drop_penalty # goal!
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(500, @pq.time_to_strength_change)
        assert_equal(0, penalties_ended)

        advance_clock 500
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
        assert_equal(1, penalties_ended)
    end

    # Player X1 gets a major penalty.
    # Player X2 gets a minor penalty.
    # Team X kills both.
    def test_major_and_minor_killed
        advance_clock 1200

        penalties_ended = 0

        p1 = HockeyPenalty.new('polacek', false)
        @pq.add_penalty(p1)
        @pq.update

        @pq.on_penalty_ended do |pdone|
            penalties_ended += 1
        end

        # 5 on 4, the minor ends in 2 minutes
        assert_equal(4, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)
        assert_equal(0, penalties_ended)

        advance_clock 600

        assert_equal(4, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)
        assert_equal(0, penalties_ended)

        p2 = HockeyPenalty.new('kennedy', true)
        @pq.add_penalty(p2)
        @pq.update

        # 5 on 3, but the minor still ends in 1 minute, 
        # bringing back to 5 on 4
        assert_equal(3, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change) 
        assert_equal(0, penalties_ended)

        advance_clock 600

        # 5 on 4 with 4 minutes left on the major
        assert_equal(4, @pq.strength)
        assert_equal(2400, @pq.time_to_strength_change)
        assert_equal(1, penalties_ended)

        advance_clock 2400

        # All penalties now over
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
        assert_equal(2, penalties_ended)
    end

    # Player X1 gets a major penalty.
    # Player X2 gets a minor penalty.
    # A goal is scored during the time both penalties are active.
    def test_major_and_minor_not_killed
        # T=18:00 major penalty -> 5 on 4 for next 5 min
        advance_clock 1200
        p1 = HockeyPenalty.new('zajac', true)
        @pq.add_penalty(p1)
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(3000, @pq.time_to_strength_change)
        
        # T=17:30 minor penalty -> 5 on 3 for next 2 min
        advance_clock 300
        p2 = HockeyPenalty.new('boileau', false)
        @pq.add_penalty(p2)
        @pq.update

        assert_equal(3, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)

        # T=17:15 goal!
        advance_clock 150
        @pq.drop_penalty        
        @pq.update
        

        # The minor is dismissed. There is still 4:15 left on the major.
        assert_equal(4, @pq.strength)
        assert_equal(2550, @pq.time_to_strength_change)

    end

    # Player X1 gets a major penalty.
    # Player X2 gets a minor penalty.
    # The minor is killed, but a goal is scored 
    # while the major is still active.
    def test_minor_killed_major_not_killed
        # T=18:00 minor
        advance_clock 1200
        p1 = HockeyPenalty.new('zajac', false)
        @pq.add_penalty(p1)
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)

        # T=17:30 major
        advance_clock 300
        assert_equal(4, @pq.strength)
        assert_equal(900, @pq.time_to_strength_change)

        p2 = HockeyPenalty.new('boileau', true)
        @pq.add_penalty(p2)
        @pq.update

        assert_equal(3, @pq.strength)
        assert_equal(900, @pq.time_to_strength_change)

        # T=16:00 minor expires
        advance_clock 900
        assert_equal(4, @pq.strength)
        assert_equal(2100, @pq.time_to_strength_change)

        # T=15:30 goal
        advance_clock 300
        assert_equal(4, @pq.strength)
        assert_equal(1800, @pq.time_to_strength_change)

        @pq.drop_penalty
        @pq.update
        assert_equal(4, @pq.strength)
        assert_equal(1800, @pq.time_to_strength_change)

        # T=12:30 major expires
        advance_clock 1800
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
    end

    # Player X gets assessed a double minor.
    # Team X kills it.
    def test_double_minor_killed
        # T=18:00 double minor
        advance_clock 1200
        p1 = HockeyPenalty.new('kennedy', false)
        p2 = HockeyPenalty.new('kennedy', false)
        @pq.add_penalty(p1)
        @pq.add_penalty(p2)
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(2400, @pq.time_to_strength_change)

        # T=17:00 double minor in effect
        advance_clock 600
        assert_equal(4, @pq.strength)
        assert_equal(1800, @pq.time_to_strength_change)

        # T=16:00 first half expires
        advance_clock 600
        assert_equal(4, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)

        # T=15:00 second half in effect
        advance_clock 600
        assert_equal(4, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        # T=14:00 double minor expired
        advance_clock 600
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
    end

    # Player X gets assessed a double minor.
    # A goal is scored during the first half.
    def test_double_minor_goal_first
        # T=18:00 double minor
        advance_clock 1200

        p1 = HockeyPenalty.new('kennedy', false)
        p2 = HockeyPenalty.new('kennedy', false)
        @pq.add_penalty(p1)
        @pq.add_penalty(p2)
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(2400, @pq.time_to_strength_change)

        # T=17:30 minor in effect 3:30 left
        advance_clock 300
        assert_equal(4, @pq.strength)
        assert_equal(2100, @pq.time_to_strength_change)

        # T=17:00 goal
        advance_clock 300
        assert_equal(4, @pq.strength)
        assert_equal(1800, @pq.time_to_strength_change)

        @pq.drop_penalty
        @pq.update
        assert_equal(4, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)

        # T=16:00 minor in effect 1:00 left
        advance_clock 600
        assert_equal(4, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        # T=15:00 minor expires
        advance_clock 600
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
    end

    # Player X gets assessed a double minor.
    # A goal is scored during the second half.
    def test_double_minor_goal_second
        # T=18:00 double minor
        advance_clock 1200

        p1 = HockeyPenalty.new('kennedy', false)
        p2 = HockeyPenalty.new('kennedy', false)
        @pq.add_penalty(p1)
        @pq.add_penalty(p2)
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(2400, @pq.time_to_strength_change)

        # T=17:00 3 min left
        advance_clock 600
        assert_equal(4, @pq.strength)
        assert_equal(1800, @pq.time_to_strength_change)

        # T=16:00 2 min left
        advance_clock 600
        assert_equal(4, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)

        # T=15:00 1 min left
        advance_clock 600
        assert_equal(4, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        # T=15:00 GOAL!
        @pq.drop_penalty
        @pq.update

        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
    end

    # Players X1, X2, and X3 all get minor penalties.
    # They "stack up" so X3's penalty does not begin until later.
    def test_stacked_minors_independent_no_goal
        # T=18:00 penalty 1 minor
        advance_clock 1200
        p1 = HockeyPenalty.new('kennedy', false)
        @pq.add_penalty(p1)
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)

        # T=17:45 1:45 of 5 on 4
        advance_clock 150
        @pq.update
        assert_equal(4, @pq.strength)
        assert_equal(1050, @pq.time_to_strength_change)

        # T=17:30 penalty 2 minor
        advance_clock 150
        assert_equal(4, @pq.strength)
        assert_equal(900, @pq.time_to_strength_change)

        p2 = HockeyPenalty.new('polacek', false)
        @pq.add_penalty(p2)
        @pq.update

        # T=17:30 1:30 of 5 on 3
        assert_equal(3, @pq.strength)
        assert_equal(900, @pq.time_to_strength_change)
        
        # T=17:15 1:15 of 5 on 3
        advance_clock 150
        assert_equal(3, @pq.strength)
        assert_equal(750, @pq.time_to_strength_change)

        # T=16:30 penalty 3 minor
        advance_clock 450
        assert_equal(3, @pq.strength)
        assert_equal(300, @pq.time_to_strength_change)

        p3 = HockeyPenalty.new('rabbani', false)
        @pq.add_penalty(p3)
        @pq.update

        # T=16:30 1:00 left of 5 on 3
        assert_equal(3, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        # T=16:00 penalty 1 expired, :30 left of 5 on 3
        advance_clock 300
        assert_equal(3, @pq.strength)
        assert_equal(300, @pq.time_to_strength_change)

        # T=15:30 penalty 2 expiored, 1:30 5 on 4
        advance_clock 300
        assert_equal(4, @pq.strength)
        assert_equal(900, @pq.time_to_strength_change)
    end

    # Players X1 and X2 both get double minors.
    # Later, X3 also gets a double minor. These penalties stack.
    # Somehow, Team X kills all these penalties.
    def test_stacked_double_minors_no_goal
        # 18:00 begin double minor 1
        advance_clock 1200

        @pq.add_penalty(HockeyPenalty.new('polacek', false))
        @pq.add_penalty(HockeyPenalty.new('polacek', false))
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(2400, @pq.time_to_strength_change)

        # 17:00 3 min left of PP
        advance_clock 600

        assert_equal(4, @pq.strength)
        assert_equal(1800, @pq.time_to_strength_change)

        # 17:00 begin double minor 2
        @pq.add_penalty(HockeyPenalty.new('kennedy', false))
        @pq.add_penalty(HockeyPenalty.new('kennedy', false))
        @pq.update

        # 17:00 3 min left of 5-on-3
        assert_equal(3, @pq.strength)
        assert_equal(1800, @pq.time_to_strength_change)

        # 16:00 2 min left of 5-on-3
        advance_clock 600
        assert_equal(3, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)
    
        # 16:00 queue double minor 3
        @pq.add_penalty(HockeyPenalty.new('rabbani', false))
        @pq.add_penalty(HockeyPenalty.new('rabbani', false))
        @pq.update

        # 16:00 3 min left of 5-on-3
        assert_equal(3, @pq.strength)
        assert_equal(1800, @pq.time_to_strength_change)

        # 14:00 double minor 1 ends, double minor 3 begins, still 5 on 3
        advance_clock 1200

        assert_equal(3, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change) 

        # 13:00 double minor 2 ends, back to 5 on 4
        advance_clock 600
        assert_equal(4, @pq.strength)
        assert_equal(1800, @pq.time_to_strength_change)

        # 10:00 double minor 3 ends, back to full strength
        advance_clock 1800
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
    end

    # Players X1 and X2 both have double minors.
    # A goal is scored. No one is penalized again.
    def test_coincident_double_minors_goal_1
        # 18:00 double minors 1 and 2
        advance_clock 1200
        @pq.add_penalty(HockeyPenalty.new('zajac', false))
        @pq.add_penalty(HockeyPenalty.new('zajac', false))

        @pq.add_penalty(HockeyPenalty.new('boileau', false))
        @pq.add_penalty(HockeyPenalty.new('boileau', false))

        @pq.update

        assert_equal(3, @pq.strength)
        assert_equal(2400, @pq.time_to_strength_change)

        # 17:00 goal!
        advance_clock 600
        
        @pq.drop_penalty
        @pq.update

        # 17:00 there were 3 minutes left of 5 on 3 at this point.
        # One of the double minors now ends in 2 minutes.
        # This means 2 min left of 5 on 3, and 1 min of PP after that.
        assert_equal(3, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)

        advance_clock 1200
        assert_equal(4, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        advance_clock 600
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
    end

    def test_coincident_double_minors_goal_2
        # 18:00 double minors 1 and 2
        advance_clock 1200
        @pq.add_penalty(HockeyPenalty.new('zajac', false))
        @pq.add_penalty(HockeyPenalty.new('zajac', false))

        @pq.add_penalty(HockeyPenalty.new('boileau', false))
        @pq.add_penalty(HockeyPenalty.new('boileau', false))

        @pq.update

        assert_equal(3, @pq.strength)
        assert_equal(2400, @pq.time_to_strength_change)

        # 15:00 goal!
        advance_clock 1800
        
        @pq.drop_penalty
        @pq.update

        # 1 min left of 5 on 3 becomes 1 min left of PP after goal
        assert_equal(4, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        advance_clock 600
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
    end

    # Players X1 and X2 both have minors.
    # A goal is scored, X1 leaves, then X1 is penalized again.
    def test_coincident_minors_goal_penalty_1
        # 18:00 minors 1 and 2
        advance_clock 1200
        @pq.add_penalty(HockeyPenalty.new('zajac', false))
        @pq.add_penalty(HockeyPenalty.new('boileau', false))
        @pq.update

        assert_equal(3, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)

        # 17:00 goal!
        advance_clock 600
        
        assert_equal(3, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        @pq.drop_penalty
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        # 16:30 first player penalized again
        advance_clock 300
        assert_equal(4, @pq.strength)
        assert_equal(300, @pq.time_to_strength_change)

        @pq.add_penalty(HockeyPenalty.new('zajac', false))
        @pq.update

        assert_equal(3, @pq.strength)
        assert_equal(300, @pq.time_to_strength_change)

        # 16:00 second original penalty ends
        advance_clock 300
        assert_equal(4, @pq.strength)
        assert_equal(900, @pq.time_to_strength_change)
        
        # 14:30 third penalty ends
        advance_clock 900
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)
    end

    # Players X1 and X2 both have double minors.
    # A goal is scored, X2 leaves, then X2 is penalized again.
    def test_coincident_minors_goal_penalty_2
        # 18:00 minors 1 and 2
        advance_clock 1200
        @pq.add_penalty(HockeyPenalty.new('zajac', false))
        @pq.add_penalty(HockeyPenalty.new('boileau', false))
        @pq.update

        assert_equal(3, @pq.strength)
        assert_equal(1200, @pq.time_to_strength_change)

        # 17:00 goal!
        advance_clock 600
        
        assert_equal(3, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        @pq.drop_penalty
        @pq.update

        assert_equal(4, @pq.strength)
        assert_equal(600, @pq.time_to_strength_change)

        # 16:30 first player penalized again
        advance_clock 300
        assert_equal(4, @pq.strength)
        assert_equal(300, @pq.time_to_strength_change)

        @pq.add_penalty(HockeyPenalty.new('boileau', false))
        @pq.update

        assert_equal(3, @pq.strength)
        assert_equal(300, @pq.time_to_strength_change)

        # 16:00 second original penalty ends
        advance_clock 300
        assert_equal(4, @pq.strength)
        assert_equal(900, @pq.time_to_strength_change)
        
        # 14:30 third penalty ends
        advance_clock 900
        assert_equal(5, @pq.strength)
        assert_equal(0, @pq.time_to_strength_change)

    end

    protected

    def advance_clock(t)
        @tc.advance(t) do
            @pq.update
        end
    end
end
