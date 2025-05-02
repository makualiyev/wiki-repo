# Development and Tooling

## WSL

The development from the start was solely on Windows machine - a single laptop with 8 to 16 GB RAM, but recently I started to specifically target Linux machines to ship the project on Ubuntu server or at least WSL. Thus it is easier to interact with the system, get a hold of monitoring processes, using orchestrators containers and etc.

## PostgreSQL

The main RDBMS we use for day to day activities, for our purposes it is the most decent option.

### Installing PostgreSQL on WSL

Start by reading this manual from the official [PostgreSQL site](https://www.postgresql.org/download/linux/ubuntu/) and from [Neon Tech](https://neon.tech/postgresql/postgresql-getting-started/install-postgresql-linux), also check this one from [Ubuntu Community Wiki](https://help.ubuntu.com/community/PostgreSQL). AFAIK there are some versions of APT packages for Postgres such as Debian/Ubuntu version and PGDG (*PostgreSQL Global Development Group*) that provide different kind of experience. Debian/Ubuntu packages have only one specific version for respective distro version, for example Ubuntu 22.04 supports only Postgres version 14 (check this link for [the explanation](https://askubuntu.com/questions/1456014/how-to-upgrade-postgresql-from-14-to-15-on-ubuntu-22-04)). As my WSL installation of Ubuntu had version 22.04 and I wanted to install Postgres version 16, I used PGDG package following manual steps described in the official documentation.

- First we `sudo apt update` and `sudo apt upgrade`, then check if PostgreSQL is already installed on the system by checking by this command `sudo find /etc -name "postgres*"`
- Then we install `sudo apt install curl ca-certificates wget gnupg2`
- Then we do the following:

```bash
# Import the repository signing key:
sudo apt install curl ca-certificates
sudo install -d /usr/share/postgresql-common/pgdg
sudo curl -o /usr/share/postgresql-common/pgdg/apt.postgresql.org.asc --fail https://www.postgresql.org/media/keys/ACCC4CF8.asc

# Create the repository configuration file:
. /etc/os-release
sudo sh -c "echo 'deb [signed-by=/usr/share/postgresql-common/pgdg/apt.postgresql.org.asc] https://apt.postgresql.org/pub/repos/apt $VERSION_CODENAME-pgdg main' > /etc/apt/sources.list.d/pgdg.list"

# Update the package lists:
sudo apt update

# Install the latest version of PostgreSQL:
# If you want a specific version, use 'postgresql-17' or similar instead of 'postgresql'
sudo apt -y install postgresql
```

- Then we get it up and running, quick check is by `htop` ing it:

![postgresql-htop-representation](../assets/guides/postgresql-htop-repres.png)

- The `hello, world` moment for the installation is connecting via a client to the server using default postgres user: `sudo -u postgres psql` and we are done

### Administrating Postgres

- Adding a reader user is the same thing as adding a user in Linux other than `root`, as it is out of best practices domain to use `root` on everyday basis.

------------------------------------------------
[<- Table of Contents](../README.md)
