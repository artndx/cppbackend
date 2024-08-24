curl -i -v -X GET "http://127.0.0.1:8080/"

curl -i -v -X POST "http://127.0.0.1:8080/api/v1/game/join" -H "Content-Type: application/json" --data "{\"userName\": \"Harry\", \"mapId\": \"map1\"}"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/game/state" -H "Content-Type: application/json" -H "Authorization: Bearer 000000000000000044e3d12c96944c07"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/game/players" -H "Content-Type: application/json" -H "Authorization: Bearer 000000000000000044e3d12c96944c07"

curl -i -v -X POST "http://127.0.0.1:8080/api/v1/game/player/action" -H "Content-Type: application/json" -H "Authorization: Bearer 000000000000000044e3d12c96944c07" --data "{\"move\": \"R\"}"