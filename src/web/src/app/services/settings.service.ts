import { Injectable } from '@angular/core';
import { Channel } from 'app/modules/client/utils/channel';

@Injectable({
  providedIn: 'root',
})
export class SettingService {
  settings$ = Channel.exec<Settings>('settings.getSettings');
  constructor() {}
}

export interface Settings {
  allowShowPackageName: boolean;
  allowSwitchRegion: boolean;
  autoInstall: boolean;
  defaultRegion: string;
  metadataServer: string;
  operationServerMap: { [key: string]: string };
  remoteDebug: boolean;
  supportAot: boolean;
  supportSignIn: boolean;
  themeName: string;
  upyunBannerVisible: boolean;
}
