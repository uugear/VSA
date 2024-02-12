#!/bin/bash
# /etc/init.d/vsa_btns

### BEGIN INIT INFO
# Provides:          vsa_btns
# Required-Start:    $all
# Required-Stop:     $all
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Vivid Unit screen assistant initialize script
# Description:       This survice runs vsa_btns process in the background and handles touch button events.
### END INIT INFO

case "$1" in
    start)
        echo "Running vsa_btns..."
				export DISPLAY=":0.0"
        /usr/bin/vsa_btns > /var/log/vsa_btns.log 2>&1 &
				sleep 1
				vsa_btns_pid=$(pidof vsa_btns)
				echo $vsa_btns_pid > /var/run/vsa_btns.pid
        ;;
    stop)
        echo "Stopping vsa_btns..."
				vsa_btns_pid=$(cat /var/run/vsa_btns.pid)
				kill -9 $vsa_btns_pid
        ;;
		status)
				if pidof myservice > /dev/null; then
					echo "vsa_btns is running"
        else
					echo "vsa_btns is not running"
        fi
				;;
    *)
        echo "Usage: /etc/init.d/vsa_btns start|stop|status"
        exit 1
        ;;
esac

exit 0