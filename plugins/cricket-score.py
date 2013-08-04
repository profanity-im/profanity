import prof
import urllib2
import json
import time

score_url = "http://api.scorescard.com/?type=score&teamone=Australia&teamtwo=England"

# hooks

def prof_on_start():
    req = urllib2.Request(score_url, None, {'Content-Type': 'application/json'})
    f = urllib2.urlopen(req)
    response = f.read()
    f.close()
    result_json = json.loads(response);
    batting1 = result_json['cb1']
    batting2 = result_json['cb2']
    team1_first = result_json['t1FI']
    team2_first = result_json['t2FI']

    prof.cons_show(batting1)
    prof.cons_show(batting2)
    prof.cons_show(team1_first)
    prof.cons_show(team2_first)

    prof.notify(team2_first, 5000, "Cricket score")
