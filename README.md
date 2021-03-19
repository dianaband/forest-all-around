# Conversation about the ROOT @ SEMA storage, Seoul

## platformio

## small IoT objects in the wireless network, LPWAN

### wirelessly network
### MIDI-like
### spacial

- 14 speakers
- 1 gas-tank
- 1 float

- 1 osc client (Root node) <-- Puredata patch to control this.
- 1 monitor agent to monitor status of the net.

- 1 Pd patch receives OSC msg. over Bluetooth serial dev. (CNMAT/OSC, slip-encoded OSC) [from this proj.](https://github.com/applecargo/midi_keyboard) sends out to wired USB serial dev. (osc client) this client translates these msg. for the mesh.

## !!NOTE!! (2019 12 16)

  - esp32 postman was added.. in yesterday's performance.
  - well.. but the network became quite unstable.. it was kind of unsatisfactory.
  - not sure is this esp32 injection caused this or not?
  - (or... also note that we have enabled ROOT/CONSTAINS_ROOT at some [point](https://github.com/dianaband/crickets/commit/33df8c26b74fe89994ad45532576014340101177))
  - woo... anyway this is really frustrating .. always..
  - i need a proper working-good system to build up sth. reliable.
  - esp32 based mesh is better? (ESP-MESH)
  - we need to find a solution ASAP.
