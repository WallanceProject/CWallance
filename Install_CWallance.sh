#!/bin/bash

# Install CWallance Project
# Command: ./Install_CWallance.sh



# -----------------
# LINUX ENVIRONMENT
# -----------------

# Update Ubuntu
sudo apt-get update

# Install modules
sudo apt-get install -y net-tools \
openssh-server \
wget \
nmap \
zip

# Create Wallance Project Directory
mkdir -p $HOME/WallanceProject



# ------------
# SUBLIME TEXT
# ------------

sudo apt-get install -y gnupg
sudo apt-get install -y libgtk2.0-0
sudo wget -qO - https://download.sublimetext.com/sublimehq-pub.gpg | sudo apt-key add -
sudo echo "deb https://download.sublimetext.com/ apt/stable/" | sudo tee /etc/apt/sources.list.d/sublime-text.list
sudo apt-get update
sudo apt-get install -y sublime-text



# ----------------
# COMPILATER TOOLS
# ----------------

# Install tools
sudo apt-get install -y gcc g++ \
gcc-arm-linux-gnueabihf \
g++-arm-linux-gnueabihf \
gcc-arm-linux-gnueabi \
g++-arm-linux-gnueabi


# -------
# SQLITE3
# -------

# Install SQLite3 - Command line (for IDE only)
cd $HOME/WallanceProject
sudo apt-get install -y sqlite3

# Install SQLite3 - C/C++ Libraries
export DEBIAN_FRONTEND=noninteractive
sudo apt-get install -y tclsh
wget https://www.sqlite.org/src/zip/sqlite.zip
unzip sqlite.zip -d SQLite3
mv SQLite3/sqlite/* SQLite3/
rm -r SQLite3/sqlite
rm sqlite.zip

# SQLite3 Compilation Option
export CFLAGS='-DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1'

# Compile SQLite3 for ARM architecture
cd $HOME/WallanceProject/SQLite3
./configure --host=arm-linux CC=arm-linux-gnueabihf-gcc
make clean && make


# -----------------
# GRAFANA INTERFACE
# -----------------

# Install Grafana Interface - Install MySQL
cd $HOME/WallanceProject
sudo apt-get install -y mysql-server
sudo service mysql start
sudo mysql -e "CREATE USER 'grafanaReader'@'localhost'"
sudo mysql -e "GRANT ALL PRIVILEGES ON *.* TO 'grafanaReader'@'localhost';"
sudo mysql -e "FLUSH PRIVILEGES;"

# Install Grafana Interface - Install Web Browser (Firefox)
sudo apt-get install -y firefox

# Install Grafana Interface - Install Grafana (Default Login/Password: admin/admin)
wget https://dl.grafana.com/oss/release/grafana_5.4.3_amd64.deb
sudo dpkg -i grafana_5.4.3_amd64.deb
rm grafana_5.4.3_amd64.deb



# ---------
# CWALLANCE
# ---------

# Download CWallance
cd $HOME/WallanceProject
rm -R -f $HOME/WallanceProject/CWallance
wget https://github.com/WallanceProject/CWallance/archive/master.zip -O CWallance.zip

# Unzip CWallance
unzip CWallance.zip
mv CWallance-master CWallance
rm CWallance.zip

# Enable Execution Permission of all Scripts
find $HOME/WallanceProject/CWallance/ -type f -iname "*.sh" -exec chmod +x {} \;

# Install Grafana Interface
sudo cp $HOME/WallanceProject/CWallance/Grafana/Wallance_DataSource.yaml /etc/grafana/provisioning/datasources/
sudo cp $HOME/WallanceProject/CWallance/Grafana/Wallance_Dashboard.js /usr/share/grafana/public/dashboards/

# Install Grafana Interface Buying Request Generator
sudo cp $HOME/WallanceProject/CWallance/Grafana/Grafana_Interface.desktop /usr/share/applications/
sudo apt-get install -y desktop-file-utils
sudo update-desktop-database

# Compile CWallance
cd $HOME/WallanceProject/CWallance
./Compile_CWallance.sh