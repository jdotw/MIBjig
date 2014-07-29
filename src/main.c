/* generated from net-snmp-config */
#include <net-snmp/net-snmp-config.h>
#ifdef HAVE_SIGNAL
#include <signal.h>
#endif /* HAVE_SIGNAL */
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "parse.h"

const char *app_name = "mibjig";

extern int netsnmp_running;

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

RETSIGTYPE
stop_server(UNUSED int a) {
      netsnmp_running = 0;
}

static void
usage(const char *prog)
{
    fprintf(stderr,
            "USAGE: %s [OPTIONS] WALK-OUTPUT.TXT [agentaddr]\n"
            "\n"
            "OPTIONS:\n", prog);

    fprintf(stderr,
            "  -c FILE[,...]\t\tread FILE(s) as configuration file(s)\n"
            "  -C\t\t\tdo not read the default configuration files\n"
            "\t\t\t  (config search path: %s)\n"
            "  -d\t\t\tdump all traffic\n"
            "  -D TOKEN[,...]\tturn on debugging output for the specified "
            "TOKENs\n"
            "\t\t\t   (ALL gives extremely verbose debugging output)\n"
            "  -f\t\t\tDo not fork() from the calling shell.\n"
            "  -h\t\t\tdisplay this help message\n"
            "  -H\t\t\tdisplay a list of configuration file directives\n"
            "  -L LOGOPTS\t\tToggle various defaults controlling logging:\n",
            get_configuration_directory());
    snmp_log_options_usage("\t\t\t  ", stderr);
#ifndef DISABLE_MIB_LOADING
    fprintf(stderr,
            "  -m MIB[:...]\t\tload given list of MIBs (ALL loads "
            "everything)\n"
            "  -M DIR[:...]\t\tlook in given list of directories for MIBs\n"
            "\t\t\t  (default %s)\n",
            netsnmp_get_mib_directory());
#endif /* DISABLE_MIB_LOADING */
#ifndef DISABLE_MIB_LOADING
    fprintf(stderr,
            "  -P MIBOPTS\t\tToggle various defaults controlling mib "
            "parsing:\n");
    snmp_mib_toggle_options_usage("\t\t\t  ", stderr);
#endif /* DISABLE_MIB_LOADING */
    fprintf(stderr,
            "  -v\t\t\tdisplay package version number\n"
            "  -X\t\t\tbecome an AgentX subagent\n"
            "  -x TRANSPORT\t\tconnect to master agent using TRANSPORT\n");
    fprintf(stderr,
            "\n"
            "  --option=value\tconfiguration options (e.g., to run without a configuration\n"
            "\t\t\tfile, try --rocommunity=public)\n");
    fprintf(stderr,
            "\n"
            "  (in master agent mode, community or user configuration is required, either\n"
            "  in the mibjig.conf configuration file or on the command line.)\n");
    exit(1);
}

static void
version(void)
{
    fprintf(stderr, "NET-SNMP version: %s\n", netsnmp_get_version());
    exit(0);
}

int
main (int argc, char **argv)
{
  int arg;
  char* cp = NULL;
  int dont_fork = 0, do_help = 0;
  int use_agentx = 0;

  while ((arg = getopt(argc, argv, "c:CdD:fhHL:Xp"
#ifndef DISABLE_MIB_LOADING
                       "m:M:"
#endif /* DISABLE_MIB_LOADING */
                       "n:"
#ifndef DISABLE_MIB_LOADING
                       "P:"
#endif /* DISABLE_MIB_LOADING */
                       "vx:-:")) != EOF) {
    switch (arg) {
    case '-':
      if (strcmp(optarg, "help") == 0) {
        do_help = 1;
        break;
      }
      if (strcmp(optarg, "version") == 0) {
        version();
        break;
      }
      handle_long_opt(optarg);

    case 'c':
        if (optarg != NULL) {
            netsnmp_ds_set_string(NETSNMP_DS_LIBRARY_ID,
                                  NETSNMP_DS_LIB_OPTIONALCONFIG, optarg);
        } else {
            usage(argv[0]);
        }
        break;

    case 'C':
        netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
                               NETSNMP_DS_LIB_DONT_READ_CONFIGS, 1);
        break;


    case 'd':
      netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
                             NETSNMP_DS_LIB_DUMP_PACKET, 1);
      break;

    case 'D':
      debug_register_tokens(optarg);
      snmp_set_do_debugging(1);
      break;

    case 'f':
      dont_fork = 1;
      break;

    case 'h':
      usage(argv[0]);
      break;

    case 'H':
      do_help = 1;
      break;

    case 'L':
      if (snmp_log_options(optarg, argc, argv) < 0) {
        exit(1);
      }
      break;
#ifndef DISABLE_MIB_LOADING
    case 'm':
      if (optarg != NULL) {
        setenv("MIBS", optarg, 1);
      } else {
        usage(argv[0]);
      }
      break;

    case 'M':
      if (optarg != NULL) {
        setenv("MIBDIRS", optarg, 1);
      } else {
        usage(argv[0]);
      }
      break;
#endif /* DISABLE_MIB_LOADING */

    case 'n':
      if (optarg != NULL) {
        app_name = optarg;
        netsnmp_ds_set_string(NETSNMP_DS_LIBRARY_ID,
                              NETSNMP_DS_LIB_APPTYPE, app_name);
      } else {
        usage(argv[0]);
      }
      break;

#ifndef DISABLE_MIB_LOADING
    case 'P':
      cp = snmp_mib_toggle_options(optarg);
      if (cp != NULL) {
        fprintf(stderr, "Unknown parser option to -P: %c.\n", *cp);
        usage(argv[0]);
      }
      break;
#endif /* DISABLE_MIB_LOADING */

    case 'v':
      version();
      break;

    case 'X':
      use_agentx = 1;
      break;

    case 'x':
      if (optarg != NULL) {
        netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID,
                              NETSNMP_DS_AGENT_X_SOCKET, optarg);
      } else {
        usage(argv[0]);
      }
      break;

    default:
      fprintf(stderr, "invalid option: -%c\n", arg);
      usage(argv[0]);
      break;
    }
  }

  /* 0 args? bad. */
  if (optind >= argc && !do_help) {
    fprintf(stderr, "need mib file to load\n");
    usage(argv[0]);
    exit(1);
  }

  /* too many args? bad. */
  if (optind + 1 + (use_agentx ? 0 : 1) < argc) {
    fprintf(stderr, "too many arguments - optind %d argc %d\n", optind, argc);
    usage(argv[0]);
    exit(1);
  }

  if (do_help) {
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID,
                           NETSNMP_DS_AGENT_NO_ROOT_ACCESS, 1);
  } else {
    if (use_agentx) {
      /* we are a subagent */
      netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID,
                             NETSNMP_DS_AGENT_ROLE, 1);
    }

    if (!dont_fork) {
      if (netsnmp_daemonize(1, snmp_stderrlog_status()) != 0)
        exit(1);
    }

    /* initialize tcpip, if necessary */
    SOCK_STARTUP;
  }

  /* Parse file */
  if (optind < argc) {
    char *objfile = argv[optind++];

    m_parse(objfile);

    snmp_log(LOG_INFO, "mibjig parsed configuration file %s, ready to serve\n", objfile);
  }

  if (optind < argc) {
    /*
     * We accept a single agentaddr specification on the command line.
     */
    netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID,
        NETSNMP_DS_AGENT_PORTS, argv[optind++]);
  }

  /* initialize the agent library */
  init_agent(app_name);

  /* Initialize SNMP */
  init_snmp(app_name);

  if (do_help) {
    fprintf(stderr, "Configuration directives understood:\n");
    read_config_print_usage("  ");
    exit(0);
  }

  if (!use_agentx) {
    /*
     * Note that we don't call init_mib_modules(), so we don't have to worry
     * about the standard modules interfering with the data we loaded from
     * the file.
     */
    if (init_master_agent() != 0) {
      /*
       * Some error opening one of the specified agent transports.  
       */
      snmp_log(LOG_ERR, "Server Exiting with code 1\n");
      exit(1);
    }
  }

  /* In case we received a request to stop (kill -TERM or kill -INT) */
  netsnmp_running = 1;
#ifdef SIGTERM
  signal(SIGTERM, stop_server);
#endif
#ifdef SIGINT
  signal(SIGINT, stop_server);
#endif

  /* main loop here... */
  while(netsnmp_running) {
    agent_check_and_process(1);
  }

  /* at shutdown time */
  snmp_shutdown(app_name);
  SOCK_CLEANUP;
  exit(0);
}

