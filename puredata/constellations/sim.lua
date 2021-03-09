-- color
_col = ofTable()
_col.red = 0xff0000
_col.yellow = 0xffff00
_col.green = 0x00ff00
_col.cyan = 0x00ffff
_col.blue = 0x0000ff
_col.magenta = 0xff00ff
_col.navy = 0x000080

-- pl
_pl = ofTable()
function _pl.new(mass, x0, y0, vx0, vy0, hexcolor, r, fixed)

  local o = ofTable()
  o.mass = mass
  o.pos = ofVec2f(x0, y0)
  o.vel = ofVec2f(vx0, vy0)
  o.acc = ofVec2f(0, 0)
  o.color = hexcolor
  o.r = r
  o.fixed = fixed
  o.name = "newplanet"

  function o.display()
    ofPushMatrix()
    ofTranslate(o.pos.x, o.pos.y)
    ofFill()
    ofSetHexColor(o.color)
    ofDrawCircle(0, 0, o.r)
    ofSetHexColor(0xffffff)
    ofDrawCircle(0, 0, 1)
    ofSetHexColor(0xffffff)
    local spd = math.sqrt(o.vel.x*o.vel.x + o.vel.y*o.vel.y)
    local jerk = math.sqrt(o.acc.x*o.acc.x + o.acc.y*o.acc.y)
    -- font:drawString(string.format("(%5.1f %5.1f %2.1f)", spd, jerk, o.mass), 12, 0)
    -- font:drawString(string.format("%06x", o.color), 12, 0)
    font:drawString(string.format("%.0f", o.mass), 12, 0)
    ofPopMatrix()
  end

  return o
end

-- spaces
_spaces = ofTable()

-- planets
_planets = ofTable()

-- '10 randomized planets' (TESTING)
-- for i = 1, 10 do
--   _planets[i] = _pl.new(ofRandom(0.1, 1000), ofRandom(-ofGetWidth()/2, ofGetWidth()/2), ofRandom(-ofGetHeight()/2, ofGetHeight()/2), ofRandom(100), ofRandom(100), ofRandom(255), 10, false)
-- end

-- 'sun_planet_moon'
-- _planets[1] = _pl.new(200,     0, 0, 0,   0, ofRandom(255), 10, true)
-- _planets[2] = _pl.new(10,    160, 0, 0, 240, ofRandom(255), 10, false)
-- _planets[3] = _pl.new(0.001, 140, 0, 0, 240, ofRandom(255), 10, false)

-- 'sun_planet_comet'
-- _planets[1] = _pl.new(200,      0,   0,   0,   0, ofRandom(255), 10, true)
-- _planets[2] = _pl.new(1,      150,   0,   0, 120, ofRandom(255), 10, false)
-- _planets[3] = _pl.new(0.001, -220, 130, -15, -28, ofRandom(255), 10, false)

-- 'double_double'
-- _planets[1] = _pl.new(60, -115,  -3,  0, -155,  0xcccccc, 10, false)
-- _planets[2] = _pl.new(70,  102,   0,  1,  150,  0xcccc00, 10, false)
-- _planets[3] = _pl.new(55,  -77,  -2, -1,   42,  0xcc0000, 10, false)
-- _planets[4] = _pl.new(62,  135,   0, -1,  -52,  0x00cccc, 10, false)

-- broucke_henon
-- _planets[1] = _pl.new(100, -100,         0,         0, -93.9325, _col.blue, 10, false)
-- _planets[2] = _pl.new(100,   50,  -64.7584,  -50.5328, 46.96663, _col.yellow, 10, false)
-- _planets[3] = _pl.new(100,   50,   64.7584,   50.5328, 46.96663, _col.green, 10, false)

-- 'pair'
_planets[1] = _pl.new(70, -50,  0,  1,  50,  _col.blue, 10, false)
_planets[2] = _pl.new(70,  50,  0, -1, -50,  _col.yellow, 10, false)

-- sim
_sim = ofTable()
function _sim.new()

  local o = ofTable()
  o.G = 10000
  -- o.MAX_DELTA = 0.03
  -- o.STEPS = 50
  o.BOUNDED = true

  function o.getPositions()
    local pos = ofTable()
    for i = 1, #_planets do
      pos[i] = _planets[i].pos
    end
    return pos
  end

  function o.getVelocities()
    local vel = ofTable()
    for i = 1, #_planets do
      vel[i] = _planets[i].vel
    end
    return vel
  end

  function o.calculateAcceleration(pos)
    local acc = ofTable()
    for i = 1, #_planets do
      acc[i] = ofVec2f.zero()
    end
    for i = 1, #_planets do
      for j = 1, i do
        local d = pos[j] - pos[i]
        local r2 = d.x*d.x + d.y*d.y
        local d_norm = d / math.sqrt(d.x*d.x + d.y*d.y)

        local f = d_norm * o.G * _planets[i].mass * _planets[j].mass / r2

        acc[i] = acc[i] + f / _planets[i].mass
        acc[j] = acc[j] - f / _planets[j].mass
      end
    end
    return acc
  end

  function o.calculateVelocities(acc, dt)
    local vel = ofTable()
    for i = 1, #_planets do
      vel[i] = _planets[i].vel + acc[i]*dt
    end
    return vel
  end

  function o.calculatePositions(vel, dt)
    local pos = ofTable()
    for i = 1, #_planets do
      pos[i] = _planets[i].pos + vel[i]*dt
    end
    return pos
  end

  function o.updateVelocities(vel)
    for i = 1, #_planets do
      if _planets[i].fixed == false then
        _planets[i].vel = vel[i]
        if _planets[i].pos.x < -ofGetWidth()/2 or _planets[i].pos.x > ofGetWidth()/2 then
          _planets[i].vel.x = -_planets[i].vel.x
        end
        if _planets[i].pos.y < -ofGetHeight()/2 or _planets[i].pos.y > ofGetHeight()/2 then
          _planets[i].vel.y = -_planets[i].vel.y
        end
      end
    end
  end

  function o.updatePositions(pos)
    for i = 1, #_planets do
      if _planets[i].fixed == false then
        _planets[i].pos = pos[i]
      end
    end
  end

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
    for i = 1, #_planets do
      acc[i] = (a1[i] / 6 + a2[i] / 3 + a3[i] / 3 + a4[i] / 6) * 1
      vel[i] = (v1[i] / 6 + v2[i] / 3 + v3[i] / 3 + v4[i] / 6) * 1
      -- acc[i] = acc[i] - 1;
    end

    o.updatePositions(o.calculatePositions(vel, delta))
    o.updateVelocities(o.calculateVelocities(acc, delta))
  end

  return o
end
_ss = _sim.new()

return nil
