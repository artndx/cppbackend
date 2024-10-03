curl -i -v -X GET "http://127.0.0.1:8080/"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/maps"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/maps/map1"

curl -i -v -X POST "http://127.0.0.1:8080/api/v1/game/join" -H "Content-Type: application/json" --data "{\"userName\": \"User1\", \"mapId\": \"map1\"}"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/tick" -H "Content-Type: application/json" -H "Authorization: Bearer d1f095f9ee9af3f6d44d9ac941a971a6" --data "{\"timeDelta\": 7500"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/state" -H "Content-Type: application/json" -H "Authorization: Bearer cae5c0f68773102ab31e1a68d26ac5a3"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/state" -H "Content-Type: application/json" -H "Authorization: Bearer cae5c0f68773102ab31e1a68d26ac5a3"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/players" -H "Content-Type: application/json" -H "Authorization: Bearer cae5c0f68773102ab31e1a68d26ac5a3"

curl -i -v -X POST "http://127.0.0.1:8080/api/v1/player/action" -H "Content-Type: application/json" -H "Authorization: Bearer f9ad45c1d2abdeecb3a13d52ee6c2384" --data "{\"move\": \"R\"}"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/player/state" -H "Content-Type: application/json" -H "Authorization: Bearer cae5c0f68773102ab31e1a68d26ac5a3"

# ------------------------------------------------- ------------------------------------------------- ------------------------------------------------- #

curl -i -v -X POST "http://127.0.0.1:8080/api/v1/game/join" -H "Content-Type: application/json" --data "{\"userName\": \"User1\", \"mapId\": \"map1\"}"

curl -i -v -X POST "http://127.0.0.1:8080/api/v1/game/tick" -H "Content-Type: application/json" -H "Authorization: Bearer 5600f04dc287c56217d017431f964c84" --data "{\"timeDelta\": 7500}"

curl -i -v -X GET "http://127.0.0.1:8080/api/v1/game/state" -H "Content-Type: application/json" -H "Authorization: Bearer 5600f04dc287c56217d017431f964c84"
