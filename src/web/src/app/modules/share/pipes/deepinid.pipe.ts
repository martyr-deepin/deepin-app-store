import { Pipe, PipeTransform } from '@angular/core';
import { Subject } from 'rxjs';
import { bufferTime, mergeMap, filter, map, share } from 'rxjs/operators';
import { DeepinidInfoService } from '../services/deepinid.service';

@Pipe({
  name: 'deepinid',
})
export class DeepinidPipe implements PipeTransform {
  constructor(private deepinid: DeepinidInfoService) {
    this.result$.subscribe(arr => {
      console.log(arr);
    });
  }
  query = new Subject<number>();
  result$ = this.query.pipe(
    bufferTime(100, -1, 20),
    filter(arr => arr.length > 0),
    mergeMap(async arr => {
      const list = await this.deepinid.getDeepinUserInfo([...new Set(arr.sort())]);
      const m = new Map(list.map(info => [info.uid, info]));
      arr.forEach(uid => {
        if (!m.has(uid)) {
          m.set(uid, null);
        }
      });
      console.log(m);
      return m;
    }),
    share(),
  );
  transform(uid: number): any {
    setTimeout(() => this.query.next(uid));
    return this.result$.pipe(
      filter(m => m.has(uid)),
      map(m => m.get(uid)),
    );
  }
}
