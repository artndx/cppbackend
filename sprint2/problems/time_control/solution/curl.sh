curl -i -v -X GET "http://127.0.0.1:8080/"

curl -i -v -X POST "http://127.0.0.1:8080/api/v1/game/join" -H "Content-Type: application/json" --data "{\"userName\": \"Harry\", \"mapId\": \"map1\"}"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/state" -H "Content-Type: application/json" -H "Authorization: Bearer cae5c0f68773102ab31e1a68d26ac5a3"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/state" -H "Content-Type: application/json" -H "Authorization: Bearer cae5c0f68773102ab31e1a68d26ac5a3"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/players" -H "Content-Type: application/json" -H "Authorization: Bearer cae5c0f68773102ab31e1a68d26ac5a3"

curl -i -v -X POST "http://127.0.0.1:8080/api/v1/player/action" -H "Content-Type: application/json" -H "Authorization: Bearer cae5c0f68773102ab31e1a68d26ac5a3" --data "{\"move\": \"R\"}"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/player/state" -H "Content-Type: application/json" -H "Authorization: Bearer cae5c0f68773102ab31e1a68d26ac5a3"