if type(window) ~= "userdata" then
  window = ofWindow()
end

local clock = ofClock(this, "setup")
local DV = class()
function DV:__init(x, y)
  self.x = x
  self.y = y
  self.bBeingDragged = false
  self.bOver = false
  self.radius = 4
end
local nCurveVertexes = 0
local curveVertices = {}

function M.new()
  ofWindow.addListener("setup", this)
  ofWindow.addListener("draw", this)
  ofWindow.addListener("mouseMoved", this)
  ofWindow.addListener("mouseDragged", this)
  ofWindow.addListener("mousePressed", this)
  ofWindow.addListener("mouseReleased", this)
  window:setPosition(30, 100)
  window:setSize(1024, 768)
  if ofWindow.exists then
    clock:delay(0)
  else
    window:create()
  end
end

function M.free()
  window:destroy()
  ofWindow.removeListener("setup", this)
  ofWindow.removeListener("draw", this)
  ofWindow.removeListener("mouseMoved", this)
  ofWindow.removeListener("mouseDragged", this)
  ofWindow.removeListener("mousePressed", this)
  ofWindow.removeListener("mouseReleased", this)
end

function M.setup()
  ofSetWindowTitle("polygon example")
  ofBackground(255, 255, 255, 255)
  nCurveVertexes = 7
  curveVertices = {DV(326, 209), DV(306, 279), DV(265, 331), DV(304, 383), DV(374, 383), DV(418, 209), DV(345, 279)}
end

function M.draw()
  ofFill()

--[[---------------------------------------------------------------
draw a star dynamically
use the mouse position as a pct
to calc nPoints and internal point radius
--]]---------------------------------------------------------------
  local angleChangePerPt = OF_TWO_PI / 3
  local radius = 80
  local origx = 525
  local origy = 100
  local angle = 0
  ofSetHexColor(0xa16bca)
  ofBeginShape()
  for i = 0, 3 do
      local x = origx + radius * math.cos(angle)
      local y = origy + radius * math.sin(angle)
      ofVertex(x, y)
    angle = angle + angleChangePerPt
  end
  ofEndShape()
end
