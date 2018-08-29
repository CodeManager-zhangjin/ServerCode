#!/bin/bash
#
# chkconfig: - 55 45
# description: fbdserver  
# probe: false

# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

# Check that networking is up.
if [ " ${NETWORKING}" = " no" ] ; then
    STRING=$"dvipserver: networking disabled"
    echo -n "$STRING"
    failure "$STRING"
    echo
    exit 6
fi

RETVAL=0

#progdir=/usr/local/dserver/
progdir=/home/sam/samsvn/server/db_server/
prog="dbserver_exe"

start() {
    if [ ! -x $progdir$prog ] ; then
        STRING=$"dvipserver: binary missing"
	echo -n "$STRING"
	failure "$STRING"
	echo
	exit 5
    fi
    # Start dserver.
    #    ulimit -i 16384
	  if [ -n "`/sbin/pidof $prog`" ]; then
	      pid=`/sbin/pidof $prog`
  	    echo -n "$prog(pid $pid): already running"
  	    failure $"$prog start"
  	    echo
  	    return 1
    fi
        echo -n $"Starting $prog ... "
        nohup $progdir$prog >/dev/null &
        RETVAL=$?
        usleep 1000000
    if [ -z "`/sbin/pidof $progdir$prog`" ]; then
        RETVAL=1
    fi
        [ $RETVAL -ne 0 ] && failure $"$prog startup"
        [ $RETVAL -eq 0 ] && touch /var/lock/subsys/$prog && success $"$prog startup"
        echo 
        return $RETVAL
}

stop() {
        RETVAL=0
        # Stop dserver.
        if [ ! -n "`/sbin/pidof $prog`" ] ; then
     	    echo -n "$prog: not running"
            failure $"$prog stop"
            echo
  	    return 1		
        fi
        echo -n $"Stopping $prog ... "
        pid=`/sbin/pidof -s $prog`
        if [ -n "$pid" ]; then
            kill -TERM $pid
        else
            failure $"$prog stop"
            echo
            return 1
        fi
        RETVAL=$?
        [ $RETVAL -ne 0 ] && failure $"$prog stop" 
        [ $RETVAL -eq 0 ] && rm -f /var/lock/subsys/$prog && success $"$prog stop" 
        echo
        return $RETVAL
}

status() {
         if [ -n "`/sbin/pidof $prog`" ]; then
             pid=`/sbin/pidof $prog`
             echo -n "$prog(pid $pid): already running"
         else
             echo -n "$prog: not running"
         fi
 	 echo
	 return 1
}

restart() {
          stop
	  usleep 4000000
          start
} 

# See how we were called.
case "$1" in
  start)
        start
        ;;
  stop)
       stop
       ;;
  status)
       status
       ;;
  restart)
       restart
       ;;
 condrestart)
       [ -f /var/lock/subsys/$prog ] && restart
       ;;
 *)
       echo $"Usage: $0 {start|stop|status|restart|condrestart}"
       exit 1
esac

exit $?



