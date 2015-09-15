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
    send_command "\$q:test_system.$2:$3:0#"
#    send_command "\$q:test_system.l2.tags:$2:0#"
    ;;
  adjust)
    ((num=16#$4))
    send_command "\$s:test_system.$2:$3:0:$num#"
#    send_command "\$s:test_system.l2.tags:$2:0:0:65532#"
#    send_command "\$s:test_system.l2.tags:$2:1:0:3#"
#    send_command "\$s:test_system.l2.tags:$2:2:0:3#"
#    send_command "\$s:test_system.l2.tags:$2:3:0:3#"
    ;;
  *)
    echo "************************************************************************"
    echo -e "***** \e[1;31mcreate LDom space\e[0m"
    echo -e "*****        \e[1;34m$0 create\e[0m"
    echo -e "***** ---------------------------------------------"
    echo -e "***** \e[1;31mstartup/shutdown LDom\e[0m"
    echo -e "*****        \e[1;34m$0 [startup|shutdown] [LDomID]\e[0m"
    echo -e "***** ---------------------------------------------"
    echo -e "***** \e[1;31mquery each CP parameter table\e[0m"
    echo -e "*****        \e[1;32mCPName: l2.tags  membus  membus.mapper\e[0m"
    echo -e "*****        \e[1;34m$0 query [CPName] [LDomID]\e[0m"
    echo -e "***** ---------------------------------------------"
    echo -e "***** \e[1;31mset each CP parameter table\e[0m"
    echo -e "*****        \e[1;32mmembus value: 0-nonblock; 1-block\e[0m"
    echo -e "*****        \e[1;34m$0 adjust [CPName] [LDomID] [value]\e[0m"
    echo "************************************************************************"
esac
