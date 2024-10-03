# Install python essentials

 apt update
 apt install -y python3-pip python3-venv cmake linux-tools-common linux-tools-generic linux-tools-$(uname -r)
python3 -m pip install -U pip

# Install docker

 apt-get remove -y docker docker-engine docker.io containerd runc # Clean installation
 apt-get install -y ca-certificates curl gnupg lsb-release
 mkdir -m 0755 -p /etc/apt/keyrings
 rm /etc/apt/keyrings/docker.gpg
curl -fsSL https://download.docker.com/linux/ubuntu/gpg |  gpg --dearmor -o /etc/apt/keyrings/docker.gpg
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" |  tee /etc/apt/sources.list.d/docker.list > /dev/null
 chmod a+r /etc/apt/keyrings/docker.gpg # Extra chmod to ensure the right rigths are set
 apt-get update
 apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
 groupadd docker
 usermod -aG docker $USER
newgrp docker