import fakeredis
import time
import json

from werkzeug.wrappers import Response
from flask import Flask, request

# initialize app and our cache (it'll go away after the app stops running)
app = Flask(__name__)
temps = fakeredis.FakeStrictRedis(0)


@app.route('/', methods=['GET'])
def get_temp():
    output = []
    for k in temps.keys():
        output.append(temps.get(k).decode('utf-8'))
    return Response(status=200, content_type='application/json',
                    response=json.dumps(output))


# curl -H "Content-Type: application/json" -X POST -d '{"temp":72}' http://127.0.0.1:5000/api/v1/temp
@app.route('/api/v1/temp', methods=['POST'])
def post_temp():
    data = json.loads(request.data.decode('utf-8'))
    if 'temp' in data.keys():
        temps.set(time.time(), data['temp'])
        return Response(status=200)
    return Response(status=400)


if __name__ == '__main__':
    app.run(host='0.0.0.0')
