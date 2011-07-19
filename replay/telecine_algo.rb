# The Telecine Pulldown Calculator
# Andrew Armenia, Exavideo LLC
# 2011-07-19

# target field rate = source frame rate * d/c
d = 20
c = 11

# TODO: reduce d/c to a lowest terms fraction

a = d / c
b = d % c

f = 0
p = 0

field = 'A'

while true
    print field
    
    f += 1

    if f == a
        f = 0
        p = p + b

        if p >= c
            p -= c
            print field # "pulldown" field
        end

        field = field.succ

        if p == 0
            break
        end
    end


end

puts


