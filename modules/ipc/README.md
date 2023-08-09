# README: ipc

## Brief

ipc provides .. <one line description>

## Detailed description

Provide more details on the functionality, API usage, etc

## Usefull Commands

### Router status
Allows to get the list of nodes connected to a router.

* Get router status: `curl http://localhost:8000/@/router/local | jq`
* Get router status via zenoh query `./bin/eolo_ipc_zenoh_query -t "@/router/<router-id>"`
