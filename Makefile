
PUID=$(shell id -u)
PGID=$(shell id -g)
GIT_BRANCH=$(shell git rev-parse --abbrev-ref HEAD | tr '/_' '-')
DOCKER_TAG="$(GIT_BRANCH)"
DOCKER_PULL_POLICY=always
SERVICE_NAME="hephaestus"
DOCKER_USER="hephaestus"
HEPHAESTUS_DEV_CONTAINER_NAME="hephaestus-dev"

docker-up:
	PUID=$(PUID) PGID=$(PGID) DOCKER_TAG=$(DOCKER_TAG) docker compose -f docker/docker-compose.yaml up --pull $(DOCKER_PULL_POLICY) --build $(SERVICE_NAME) -d --force-recreate --remove-orphans

COMMAND ?=
.PHONY: docker-exec
docker-exec:
	@if [ -z "${COMMAND}" ]; then \
		PUID=$(PUID) PGID=$(PGID) DOCKER_TAG=$(DOCKER_TAG) docker compose -f docker/docker-compose.yaml exec -it -u $(DOCKER_USER) $(SERVICE_NAME) zsh; \
	else \
		PUID=$(PUID) PGID=$(PGID) DOCKER_TAG=$(DOCKER_TAG) docker compose -f docker/docker-compose.yaml exec -it -u $(DOCKER_USER) $(SERVICE_NAME) zsh -c 'source ~/.zshrc && $(COMMAND)'; \
	fi

# log into the container
.PHONY: docker-attach docker-nvim
docker-attach:
	make docker-exec

docker-nvim:
	make docker-exec COMMAND=/usr/share/nvim-linux-x86_64/bin/nvim

# Configure attach container for VSCode.
# VSCode stores those in different places for different OSs.
UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
	CONFIG_DEST="${HOME}/Library/Application Support/Code/User/globalStorage/ms-vscode-remote.remote-containers/nameConfigs"
endif
ifeq ($(UNAME),Linux)
	CONFIG_DEST="${HOME}/.config/Code/User/globalStorage/ms-vscode-remote.remote-containers/nameConfigs"
endif
configure-attach-container:
	mkdir -p "$(CONFIG_DEST)"
	ln -sf $(shell pwd)/.devcontainer/nameConfigs/hephaestus-dev.json $(CONFIG_DEST)/$(HEPHAESTUS_DEV_CONTAINER_NAME).json

# Variables
SKIP := "*.0,*.bsd-3-clause,*.mit,*.zlib,*.drawio,*.lock,externals"
IGNORE := "lama,LAMA,HomeState,ConnectT,parana,SoM,Collet,thirdparty"

.PHONY: spellcheck
spellcheck:
	codespell \
		--skip $(SKIP) \
		--ignore-words-list $(IGNORE)

.PHONY: spellfix
spellfix:
	codespell \
		--skip $(SKIP) \
		--ignore-words-list $(IGNORE) \
		-w
