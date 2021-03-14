-- color
_col = ofTable()
_col.red = 0xff0000
_col.yellow = 0xffff00
_col.green = 0x00ff00
_col.cyan = 0x00ffff
_col.blue = 0x0000ff
_col.magenta = 0xff00ff
_col.navy = 0x000080

-- bin
local bin = ofTable()
function bin.new()
  local o = ofTable()

  -- pl
  o.pl = ofTable()
  function o.pl.new(mass, x0, y0, vx0, vy0, hexcolor, r, fixed)
    local l = ofTable()
    l.mass = mass
    l.pos = ofVec2f(x0, y0)
    l.vel = ofVec2f(vx0, vy0)
    l.acc = ofVec2f(0, 0)
    l.color = hexcolor
    l.r = r
    l.fixed = fixed
    l.name = "just a planet"

    function l.display()
      ofPushMatrix()
      ofTranslate(l.pos.x, l.pos.y)
      ofFill()
      ofSetHexColor(l.color)
      ofDrawCircle(0, 0, l.r)
      ofSetHexColor(0xffffff)
      ofDrawCircle(0, 0, 1)
      ofSetHexColor(0xffffff)
      local spd = math.sqrt(l.vel.x*l.vel.x + l.vel.y*l.vel.y)
      local jerk = math.sqrt(l.acc.x*l.acc.x + l.acc.y*l.acc.y)
      -- font:drawString(string.format("(%5.1f %5.1f %2.1f)", spd, jerk, l.mass), 12, 0)
      -- font:drawString(string.format("%06x", l.color), 12, 0)
      -- font:drawString(string.format("%.0f", l.mass), 12, 0)
      -- local ang = l.pos.angle(l.pos, ofVec2f(1, 0))
      -- font:drawString(string.format("%.0f", ang), 12, 0)
      ofPopMatrix()
    end

    return l
  end

  -- constellations (initial state of planets)
  o.constellations = ofTable()
  function o.constellations.summon(i)
    o.planets = nil --needed? for garbage collection?
    o.planets = ofTable()
    if i == 1 then
      o.planets[1] = o.pl.new(70, -50,  0,  1,  50,  _col.blue, 10, false)
      o.planets[2] = o.pl.new(70,  50,  0, -1, -50,  _col.yellow, 10, false)
    elseif i == 2 then
      o.planets[1] = o.pl.new(100, -100, 0, 0, -93.9325, _col.blue, 10, false)
      o.planets[2] = o.pl.new(100, 50, -64.7584, -50.5328, 46.9666, _col.yellow, 10, false)
      o.planets[3] = o.pl.new(100, 50, 64.7584, 50.5328, 46.9666, _col.green, 10, false)
    elseif i == 3 then
      o.planets[1] = o.pl.new(60, -115, -3, 0, -155, _col.blue, 10, false)
      o.planets[2] = o.pl.new(70, 102, 0, 1, 150, _col.yellow, 10, false)
      o.planets[3] = o.pl.new(55, -77, -2, -1, 42, _col.green, 10, false)
      o.planets[4] = o.pl.new(62, 135, 0, -1, -52, _col.cyan, 10, false)
    end
  end
  o.constellations.summon(1)

  -- gravitation constant (fake)
  o.G = 10000

  -- max. delta : small delta is helpful to get stable simulation.
  -- o.MAX_DELTA = 0.03

  -- steps : for you to fine grain 'delta' further, simulate many more times before display. --> higher load, better result.
  -- o.STEPS = 50

  -- this 'bin' (=world) is bounded? (bouncing @ outlining edges?)
  o.BOUNDED = true

  function o.getPositions()
    local pos = ofTable()
    for i = 1, #o.planets do
      pos[i] = o.planets[i].pos
    end
    return pos
  end

  function o.getVelocities()
    local vel = ofTable()
    for i = 1, #o.planets do
      vel[i] = o.planets[i].vel
    end
    return vel
  end

  function o.calculateAcceleration(pos)
    local acc = ofTable()
    for i = 1, #o.planets do
      acc[i] = ofVec2f.zero()
    end
    for i = 1, #o.planets do
      for j = 1, i do
        local d = pos[j] - pos[i]
        local r2 = d.x*d.x + d.y*d.y
        local d_norm = d / math.sqrt(d.x*d.x + d.y*d.y)

        local f = d_norm * o.G * o.planets[i].mass * o.planets[j].mass / r2

        acc[i] = acc[i] + f / o.planets[i].mass
        acc[j] = acc[j] - f / o.planets[j].mass
      end
    end
    return acc
  end

  function o.calculateVelocities(acc, dt)
    local vel = ofTable()
    for i = 1, #o.planets do
      vel[i] = o.planets[i].vel + acc[i]*dt
    end
    return vel
  end

  function o.calculatePositions(vel, dt)
    local pos = ofTable()
    for i = 1, #o.planets do
      pos[i] = o.planets[i].pos + vel[i]*dt
    end
    return pos
  end

  function o.updateVelocities(vel)
    for i = 1, #o.planets do
      if o.planets[i].fixed == false then
        o.planets[i].vel = vel[i]
        -- bouncing @ edges?
        if o.BOUNDED then
          if o.planets[i].pos.x < -ofGetWidth()/2 or o.planets[i].pos.x > ofGetWidth()/2 then
            o.planets[i].vel.x = -o.planets[i].vel.x
          end
          if o.planets[i].pos.y < -ofGetHeight()/2 or o.planets[i].pos.y > ofGetHeight()/2 then
            o.planets[i].vel.y = -o.planets[i].vel.y
          end
        end
      end
    end
  end

  function o.updatePositions(pos)
    for i = 1, #o.planets do
      if o.planets[i].fixed == false then
        o.planets[i].pos = pos[i]
      end
    end
  end

  -- Runge-Kutta 4th-order
  function o.simulate(delta)
    local v1 = o.getVelocities()
    local p1 = o.getPositions()
    local a1 = o.calculateAcceleration(p1)

    local v2 = o.calculateVelocities(a1, delta / 2)
    local p2 = o.calculatePositions(v1, delta / 2)
    local a2 = o.calculateAcceleration(p2)

    local v3 = o.calculateVelocities(a2, delta / 2)
    local p3 = o.calculatePositions(v2, delta / 2)
    local a3 = o.calculateAcceleration(p3)

    local v4 = o.calculateVelocities(a3, delta)
    local p4 = o.calculatePositions(v3, delta)
    local a4 = o.calculateAcceleration(p4)

    local acc = ofTable()
    local vel = ofTable()
    for i = 1, #o.planets do
      acc[i] = a1[i] / 6 + a2[i] / 3 + a3[i] / 3 + a4[i] / 6
      vel[i] = v1[i] / 6 + v2[i] / 3 + v3[i] / 3 + v4[i] / 6
    end

    o.updatePositions(o.calculatePositions(vel, delta))
    o.updateVelocities(o.calculateVelocities(acc, delta))
  end

  function o.display()
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
    for i = 1, #o.planets do;
      o.planets[i].display();
    end;
    -- ofSetHexColor(0x000000)
    -- ofDrawLine(o.planets[1].pos.x, o.planets[1].pos.y, o.planets[2].pos.x, o.planets[2].pos.y)
    -- ofNoFill()
    -- ofDrawRectangle(o.planets[1].pos.x - 5, o.planets[1].pos.y - 5, 10, 10)
    -- ofDrawCircle(o.planets[2].pos.x, o.planets[2].pos.y, 5)
    ofPopMatrix();
  end

  return o
end

_bin = bin.new()

return nil
