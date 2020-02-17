#/bin/sh
while true; do
    sleep $((60*10))
    ssh -nNT -R 2222:localhost:22 ip.of.computer.b
done
