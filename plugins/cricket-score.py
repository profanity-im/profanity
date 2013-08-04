import prof
import urllib2
import json
import time

score_url = "http://api.scorescard.com/?type=score&teamone=Australia&teamtwo=England"

# hooks

def prof_init(version, status):
    prof.register_timed(get_scores, 10)

def get_scores():
    req = urllib2.Request(score_url, None, {'Content-Type': 'application/json'})
    f = urllib2.urlopen(req)
    response = f.read()
    f.close()
    result_json = json.loads(response);
    summary = None

    if 't1FI' in result_json.keys():
        summary = result_json['t1FI']
        prof.cons_show(result_json['t1FI'])

    if 't2FI' in result_json.keys():
        summary += "\n" + result_json['t2FI']
        prof.cons_show(result_json['t2FI'])

    if 't1SI' in result_json.keys():
        summary += "\n" + result_json['t1SI']
        prof.cons_show(result_json['t1SI'])

    if 't2SI' in result_json.keys():
        summary += "\n" + result_json['t2SI']
        prof.cons_show(result_json['t2SI'])

    if 'ms' in result_json.keys():
        summary += "\n\n" + result_json['ms']
        prof.cons_show("")
        prof.cons_show(result_json['ms'])

    prof.notify(summary, 5000, "Cricket score")
