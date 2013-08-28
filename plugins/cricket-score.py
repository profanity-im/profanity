import prof
import urllib2
import json

#score_url = "http://api.scorescard.com/?type=score&teamone=Australia&teamtwo=England"
score_url = None

summary = None

def prof_init(version, status):
    if score_url:
        prof.register_timed(get_scores, 60)
        prof.register_command("/cricket", 0, 0,
            "/cricket",
            "Get latest cricket score.",
            "Get latest cricket score.",
            cmd_cricket)

def prof_on_start():
    if score_url:
        get_scores()

def cmd_cricket():
    global score_url
    global summary
    new_summary = None

    result_json = retrieve_scores_json()

    if 'ms' in result_json.keys():
        new_summary = result_json['ms']

    prof.cons_show("")
    prof.cons_show("Cricket score:")
    if 't1FI' in result_json.keys():
        prof.cons_show("  " + result_json['t1FI'])

    if 't2FI' in result_json.keys():
        prof.cons_show("  " + result_json['t2FI'])

    if 't1SI' in result_json.keys():
        prof.cons_show("  " + result_json['t1SI'])

    if 't2SI' in result_json.keys():
        prof.cons_show("  " + result_json['t2SI'])

    summary = new_summary
    prof.cons_show("")
    prof.cons_show("  " + summary)
    prof.cons_alert()

def get_scores():
    global score_url
    global summary
    notify = None
    new_summary = None
    change = False

    result_json = retrieve_scores_json()

    if 'ms' in result_json.keys():
        new_summary = result_json['ms']
        if new_summary != summary:
            change = True

    if change:
        prof.cons_show("")
        prof.cons_show("Cricket score:")
        if 't1FI' in result_json.keys():
            notify = result_json['t1FI']
            prof.cons_show("  " + result_json['t1FI'])

        if 't2FI' in result_json.keys():
            notify += "\n" + result_json['t2FI']
            prof.cons_show("  " + result_json['t2FI'])

        if 't1SI' in result_json.keys():
            notify += "\n" + result_json['t1SI']
            prof.cons_show("  " + result_json['t1SI'])

        if 't2SI' in result_json.keys():
            notify += "\n" + result_json['t2SI']
            prof.cons_show("  " + result_json['t2SI'])

        summary = new_summary
        notify += "\n\n" + summary
        prof.cons_show("")
        prof.cons_show("  " + summary)
        prof.cons_alert()
        prof.notify(notify, 5000, "Cricket score")

def retrieve_scores_json():
    req = urllib2.Request(score_url, None, {'Content-Type': 'application/json'})
    f = urllib2.urlopen(req)
    response = f.read()
    f.close()
    return json.loads(response);
