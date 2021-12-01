# main sources
core_sources_c = [ 'src/main.c',
                   'src/profanity.c',
                   'src/log.c',
                   'src/common.c',
                   'src/database.c',
	               'src/xmpp/contact.c',
                   'src/xmpp/stanza.c',
                   'src/xmpp/chat_session.c',
                   'src/xmpp/muc.c',
                   'src/xmpp/jid.c',
                   'src/xmpp/chat_state.c',
                   'src/xmpp/resource.c',
                   'src/xmpp/roster_list.c',
                   'src/xmpp/capabilities.c',
                   'src/xmpp/session.c',
                   'src/xmpp/connection.c',
                   'src/xmpp/iq.c',
                   'src/xmpp/message.c',
                   'src/xmpp/presence.c',
                   'src/xmpp/roster.c',
                   'src/xmpp/bookmark.c',
                   'src/xmpp/blocking.c',
                   'src/xmpp/form.c',
                   'src/xmpp/avatar.c',
                   'src/xmpp/ox.c',
                   'src/event/common.c',
                   'src/event/client_events.c',
                   'src/event/server_events.c',
                   'src/ui/window.c',
                   'src/ui/core.c',
                   'src/ui/console.c',
                   'src/ui/titlebar.c',
                   'src/ui/statusbar.c',
                   'src/ui/inputwin.c',
                   'src/ui/screen.c',
                   'src/ui/notifier.c',
                   'src/ui/window_list.c',
                   'src/ui/occupantswin.c',
                   'src/ui/rosterwin.c',
                   'src/ui/chatwin.c',
                   'src/ui/mucwin.c',
                   'src/ui/privwin.c',
                   'src/ui/confwin.c',
                   'src/ui/xmlwin.c',
                   'src/ui/buffer.c',
                   'src/ui/tray.c',
                   'src/command/cmd_defs.c',
                   'src/command/cmd_funcs.c',
                   'src/command/cmd_ac.c',
                   'src/tools/parser.c',
                   'src/tools/http_common.c',
                   'src/tools/http_upload.c',
                   'src/tools/http_download.c',
                   'src/tools/bookmark_ignore.c',
                   'src/tools/autocomplete.c',
                   'src/tools/clipboard.c',
                   'src/config/files.c',
                   'src/config/conflists.c',
                   'src/config/account.c',
                   'src/config/accounts.c',
                   'src/config/tlscerts.c',
                   'src/config/preferences.c',
                   'src/config/theme.c',
                   'src/config/color.c',
                   'src/config/scripts.c',
                   'src/plugins/plugins.c',
                   'src/plugins/api.c',
                   'src/plugins/callbacks.c',
                   'src/plugins/autocompleters.c',
                   'src/plugins/themes.c',
                   'src/plugins/settings.c',
                   'src/plugins/disco.c',
                   config_h
]

sources = core_sources_c
incdir = include_directories('.', 'command', 'config', 'ui', 'plugins', 'xmpp')

# optional sources

if pgp_dep.found()
  sources += 'src/pgp/gpg.c'
endif

otr_internal_dep = declare_dependency()
if otr_dep.found()
  otr_sources = [ files('otr/otrlibv4.c',
                        'otr/otr.c',
                       )
                ]

  otr_internal_dep = declare_dependency(sources: otr_sources, include_directories: include_directories('otr'), dependencies: otr_dep)
endif
