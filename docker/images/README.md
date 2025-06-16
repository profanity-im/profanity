# Compile the image

**Docker vs Podman**

With the exception of a few advanced commands there's zero difference between Docker vs. 
Podman when it comes to most commands. Use whichever $CMD (docker/podman/buildah) that
suits you. The instructions are almost exactly the same. 

## Build images

$CMD build -t <NAME> -f Dockerfile.<version> .

<NAME> is whatever image name you'd like to choose for the built image. Normally, you
would just use the command/service name which in this case would simply be profanity.

<version> is whichever compiled image type you prefer.

The "." at the end means "Current directory", and as such is critical to the proper
functioning of the command. You will get an error if this is not included.

**Example Error**

<user@system> docker buildx build -t profanity -f ./Dockerfile.ubuntu 
ERROR: "docker buildx build" requires exactly 1 argument.
See 'docker buildx build --help'.

## Build Examples

**Docker**

The "buildx" function is introduced in more recent docker versions.

docker buildx build -t profanity.debian -f ./Dockerfile.debian .

**Podman build vs. Buildah**

Notes 
	podman has moved sub-functions of the "build" sub-command to buildah. This is to
	separate domains of function to their respective code bases: Podman starts .. 
	images, therefore it should not build them also. That said, the classic command 
    provided as an example: 

podman build -t profanity.debian -f ./Dockerfile.debian .

**Buildah**

buildah build -t profanity.debian -f ./Dockerfile.debian .

## Run the image

Please refer to the folder "../run-scripts" for example run script and README.md.

