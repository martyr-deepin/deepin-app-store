package main

import (
	"bytes"
	"os"
	"os/exec"
	"strings"
)

func RunCommand(prog string, args ...string) (string, error) {
	buf := bytes.NewBuffer(nil)
	cmd := exec.Command(prog, args...)
	cmd.Stdout = (buf)
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return "", err
	}
	return strings.TrimSpace(buf.String()), nil
}
