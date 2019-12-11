package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"strings"
	"sync"
	"time"
)

// Metadata store app-store server info
type Metadata struct {
	block      *blocklist
	debBackend *Backend
	settings   *Settings

	lastRepoUpdated time.Time
	repoApps        map[string]*cacheAppInfo

	mutex sync.Mutex
	apps  map[string]*AppBody

	methods *struct {
		GetAppIcon         func() `in:"appName" out:"path"`
		GetAppMetadataList func() `in:"appNameList" out:"json"`
		OpenApp            func() `in:"appName"`
		OnMessage          func() `in:"playload"`
	}
}

// NewMetadata create new metadata with config
func NewMetadata() *Metadata {
	m := &Metadata{}
	m.apps = make(map[string]*AppBody)
	return m
}

func (m *Metadata) getAppIcon(appName string) string {
	iconFilepath := iconFolder + "/" + appName
	app, err := m.getAppMetadata(appName)
	if nil != err {
		return ""
	}
	fmt.Println(app.Icon)
	cacheFetch(m.settings.getMetadataServer()+"/"+app.Icon, iconFilepath, time.Hour*24*30)
	return iconFilepath
}

func (m *Metadata) getAppMetadata(appName string) (*AppBody, error) {
	type result struct {
		App AppBody `json:"app"`
	}
	ret := &result{}

	api := m.settings.getMetadataServer() + "/api/app/" + appName
	err := cacheFetchJSON(ret, api, cacheFolder+"/"+appName+".json", time.Hour*24)
	return &ret.App, err
}

type cacheAppInfo struct {
	Category    string            `json:"category"`
	PackageName string            `json:"package_name"`
	LocaleName  map[string]string `json:"locale_name"`
}

func (m *Metadata) ListStorePackages() (apps map[string]*cacheAppInfo, err error) {
	stat, err := os.Stat("/var/cache/apt/pkgcache.bin")
	if nil == err {
		if stat.ModTime().Unix() < m.lastRepoUpdated.Unix() {
			return m.repoApps, nil
		}
	} else {
		// cannot get apt pkgcache file
		// just return last
		if m.lastRepoUpdated.Unix() > 0 {
			return m.repoApps, nil
		}
	}

	apps = make(map[string]*cacheAppInfo)

	// aptitude search "?installed?origin(Uos)"
	// aptitude search "?origin(Uos)"
	origin := "Uos"
	filter := fmt.Sprintf("?origin(%v)", origin)
	cmd := exec.Command("/usr/bin/aptitude", "search", filter)
	data, err := cmd.CombinedOutput()
	if nil != err {
		return apps, err
	}
	for _, line := range strings.Split(string(data), "\n") {
		fields := strings.Fields(line)
		if len(fields) < 2 {
			continue
		}
		packageName := fields[1]
		apps[packageName] = &cacheAppInfo{
			PackageName: packageName,
		}
		fullPackageName := strings.Split(packageName, ":")
		fuzzyPackageName := fullPackageName[0]
		apps[fuzzyPackageName] = &cacheAppInfo{
			PackageName: packageName,
		}
	}
	m.lastRepoUpdated = time.Now()
	m.repoApps = apps
	return apps, err
}

// GetPackageApplicationCache 获取上架的apt缓存信息
func (m *Metadata) GetPackageApplicationCache() (apps map[string]*cacheAppInfo, err error) {
	apps = make(map[string]*cacheAppInfo)
	path := "/var/lib/lastore/applications.json"

	file, err := os.Open(path)
	if nil != err {
		return
	}
	data, err := ioutil.ReadAll(file)
	if nil != err {
		return
	}
	err = json.Unmarshal(data, &apps)

	return
}
