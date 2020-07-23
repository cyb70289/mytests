#!/bin/bash -e

sudo groupadd jenkins
sudo useradd -m -d /home/jenkins -s /bin/bash -g jenkins jenkins
sudo usermod -aG sudo jenkins
sudo usermod -aG docker jenkins
sudo sh -c 'echo "jenkins ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/jenkins'
sudo mkdir /home/jenkins/.ssh
sudo cp ssh-config /home/jenkins/.ssh/config
sudo chmod 700 /home/jenkins/.ssh
sudo chown -R jenkins:jenkins /home/jenkins/.ssh
