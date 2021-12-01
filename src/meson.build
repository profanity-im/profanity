# main sources
core_sources_c = [ files( 'main.c',
                          'profanity.c',
                          'log.c',
                          'common.c',
                          'database.c',
                          'xmpp/contact.c',
                          'xmpp/stanza.c',
                          'xmpp/chat_session.c',
                          'xmpp/muc.c',
                          'xmpp/jid.c',
                          'xmpp/chat_state.c',
                          'xmpp/resource.c',
                          'xmpp/roster_list.c',
                          'xmpp/capabilities.c',
                          'xmpp/session.c',
                          'xmpp/connection.c',
                          'xmpp/iq.c',
                          'xmpp/message.c',
                          'xmpp/presence.c',
                          'xmpp/roster.c',
                          'xmpp/bookmark.c',
                          'xmpp/blocking.c',
                          'xmpp/form.c',
                          'xmpp/avatar.c',
                          'xmpp/ox.c',
                          'event/common.c',
                          'event/client_events.c',
                          'event/server_events.c',
                          'ui/window.c',
                          'ui/core.c',
                          'ui/console.c',
                          'ui/titlebar.c',
                          'ui/statusbar.c',
                          'ui/inputwin.c',
                          'ui/screen.c',
                          'ui/notifier.c',
                          'ui/window_list.c',
                          'ui/occupantswin.c',
                          'ui/rosterwin.c',
                          'ui/chatwin.c',
                          'ui/mucwin.c',
                          'ui/privwin.c',
                          'ui/confwin.c',
                          'ui/xmlwin.c',
                          'ui/buffer.c',
                          'ui/tray.c',
                          'command/cmd_defs.c',
                          'command/cmd_funcs.c',
                          'command/cmd_ac.c',
                          'tools/parser.c',
                          'tools/http_common.c',
                          'tools/http_upload.c',
                          'tools/http_download.c',
                          'tools/bookmark_ignore.c',
                          'tools/autocomplete.c',
                          'tools/clipboard.c',
                          'config/files.c',
                          'config/conflists.c',
                          'config/account.c',
                          'config/accounts.c',
                          'config/tlscerts.c',
                          'config/preferences.c',
                          'config/theme.c',
                          'config/color.c',
                          'config/scripts.c',
                          'plugins/plugins.c',
                          'plugins/api.c',
                          'plugins/callbacks.c',
                          'plugins/autocompleters.c',
                          'plugins/themes.c',
                          'plugins/settings.c',
                          'plugins/disco.c',
                        ),
                   config_h
]

sources = core_sources_c
incdir = include_directories('.', 'command', 'config', 'ui', 'plugins', 'xmpp')

# optional sources

if pgp_dep.found()
  sources += files('pgp/gpg.c')
endif

otr_internal_dep = declare_dependency()
if otr_dep.found()
  otr_sources = [ files('otr/otrlibv4.c',
                        'otr/otr.c',
                       )
                ]

  otr_internal_dep = declare_dependency(sources: otr_sources, include_directories: include_directories('otr'), dependencies: otr_dep)
endif

omemo_internal_dep = declare_dependency()
if omemo_libsignal_dep.found() and omemo_gcrypt_dep.found()
  omemo_sources = [ files('omemo/omemo.c',
                          'omemo/crypto.c',
                          'omemo/store.c',
                          'xmpp/omemo.c',
                          'tools/aesgcm_download.c'
                          )
                ]

  omemo_internal_dep = declare_dependency(sources: omemo_sources, include_directories: include_directories('omemo'))
endif
