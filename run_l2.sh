RESULT_DIR="/tmp/l2_test_report"
mkdir -p "$RESULT_DIR"

apt-get update && apt-get install -y libjsonrpccpp-dev

touch /usr/local/bin/journalctl
chmod -R 777 /usr/local/bin/journalctl
ln -s /usr/local/bin/journalctl /usr/bin/journalctl

rm -rf /opt/logs/systimemgr.log*

/usr/local/bin/sysTimeMgr -d /etc/debug.ini | tee /opt/logs/systimemgr.log.0 &

# Run L2 Test cases
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_single_instance.json test/functional-tests/tests/test_systimemgr_single_instance.py
