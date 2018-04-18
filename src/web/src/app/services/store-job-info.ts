export class StoreJobInfo {
  id = '';
  name = '';
  status = '';
  type = '';
  speed = 0;
  progress = 0.0;
  description = '';
  packages: string[] = [];
  cancelable = false;
}
