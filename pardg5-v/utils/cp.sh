#!/bin/bash

send_command() {
    echo `sleep 1 && echo $1` | socat tcp:localhost:8143 - | sed 's/[+\$]//g' | sed 's/#/\n/g'
}

case $1 in
  startup)
    send_command "\$S:$2#"
    ;;
  shutdown)
    send_command "\$T:$2#"
    ;;
  create)
    send_command "\$C#"
    ;;
  query)
    send_command "\$q:system.mem_ctrls.addr_mapper:$2:0#"
    ;;
  adjust)
    send_command "\$s:test_system.l2.tags:$2:0:0:65532#"
    send_command "\$s:test_system.l2.tags:$2:1:0:3#"
    send_command "\$s:test_system.l2.tags:$2:2:0:3#"
    send_command "\$s:test_system.l2.tags:$2:3:0:3#"
    ;;
  *)
    echo "$0 [startup|shutdown] [LDomID]"
esac
