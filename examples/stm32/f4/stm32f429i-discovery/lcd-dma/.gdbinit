target extended-remote /dev/cu.usbmodemBED9D9E1
monitor swdp_scan
attach 1
set confirm off
define lc
  load
  continue
end
