# SERVER/CLOUD SIDE
1. Login/SSH to the server and create ```read_packet.py``` file, which code/content is in ```read_packet.py``` on my git repo.

2. Run and wait for packet
    ```
    python3 read_packet.py
    ```

# RIOT SIDE

## Register experiment with 2 nodes on Saclay site
1. Submit experiment
    ```
    iotlab-experiment submit -n sensesend -d 60 -l 2,archi=m3:at86rf231+site=saclay
    ```
2. After this, it will return the experiment ID, which we will use to get the nodes list
    ```
    iotlab-experiment get -i <exp-id> -p 
    ```

## Setup border router on one node

1. Clone RIOT 
    ```
    git clone https://github.com/RIOT-OS/RIOT
    ```
2. Go to RIOT/ folder
    ```
    cd RIOT/
    ```
3.  Run this before flashing the board
    ```
    source /opt/riot.source
    ```
4. Compile the code
    ```
    make ETHOS_BAUDRATE=500000 DEFAULT_CHANNEL=23 BOARD=iotlab-m3 -C examples/gnrc_border_router clean all
    ```
5. Flash device
    ```
    iotlab-node --flash examples/gnrc_border_router/bin/iotlab-m3/gnrc_border_router.elf -l saclay,m3,<id>
    ```
6. Run border router
    ```
    sudo ethos_uhcpd.py m3-<id> tap0 2001:660:3207:04c1::1/64
    ```

## Run application on other node
1. Clone my git repo
    ```
    git clone https://github.com/tuminguyen/sensing-iot23.git
    ```
2. Go to the folder
    ```
    cd sensing-iot23/
    ```
3. Change the IPV6 address of your server on ```main.c``` file
    ```
    nano main.c
    ```
4. When the main.c is opened, change "2a05:d016:4b4:100:2323:6378:f552:8961" to to your server IPV6 address
    ```
    #define SERVER_IPV6_ADDR "2a05:d016:4b4:100:2323:6378:f552:8961"
    ```
    Save the file by pressing ```[Ctrl + X]``` then ```y``` then ```Enter```.

5.  Run this before flashing the board
    ```
    source /opt/riot.source
    ```
6. Compile code and run
    ```
    make DEFAULT_CHANNEL=23 BOARD=iotlab-m3 -C . clean all
    ```
    ```
    iotlab-node --flash bin/iotlab-m3/say2v6.elf -l saclay,m3,<id>
    ```
7. Wait until the previous command done, then we run
    ```
    nc m3-<id> 20000
    ```
8.  Press ```Enter``` so you will see
    ```
    > 
    ```
9. Basically, you can control on|off on the sensors. Here I only have **lps331ap** (temperature/pressure) and **lsm303dlhc** (accelerometer/magnetometer) sensors. The command is simply just ```lps on | off``` or ```lsm on | off```.

<u>**Link on the demo video is uploaded here on [Youtube](https://youtu.be/88y9t6HBU_U)**</u>
